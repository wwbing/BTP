#include "inferenceengine.h"
#include "yolov6.h"
#include "postprocess.h"
#include "image_utils.h"
#include "imageprocessor.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <spdlog/spdlog.h>

struct InferenceEngine::InferenceEnginePrivate
{
    void *app_ctx;  // rknn_app_context_t*
    bool initialized;
    QString lastError;
    int modelWidth;
    int modelHeight;

    InferenceEnginePrivate() : app_ctx(nullptr), initialized(false), modelWidth(640), modelHeight(640)
    {
    }

    ~InferenceEnginePrivate()
    {
        if (initialized) {
            release();
        }
    }

    bool initializeModel(const QString &modelPath)
    {
        // 分配内存
        app_ctx = malloc(sizeof(void*));
        if (!app_ctx) {
            lastError = "分配内存失败";
            return false;
        }

        // 设置工作目录确保能找到标签文件
        QString originalDir = QDir::currentPath();
        QDir::setCurrent(QCoreApplication::applicationDirPath());

        // 初始化后处理
        init_post_process();

        // 恢复原始工作目录
        QDir::setCurrent(originalDir);

        spdlog::debug("Post process初始化完成，标签已加载");

        // 初始化RKNN模型
        const char *model_path = modelPath.toUtf8().constData();
        int ret = init_yolov6_model(model_path, (rknn_app_context_t*)app_ctx);
        if (ret != 0) {
            lastError = QString("RKNN模型初始化失败，返回码: %1").arg(ret);
            spdlog::error("{}", lastError.toStdString());
            free(app_ctx);
            app_ctx = nullptr;
            return false;
        }

        // 获取模型尺寸
        rknn_app_context_t *ctx = (rknn_app_context_t*)app_ctx;
        modelWidth = ctx->model_width;
        modelHeight = ctx->model_height;

        initialized = true;
        spdlog::info("RKNN模型初始化成功，输入尺寸: {}x{}", modelWidth, modelHeight);
        return true;
    }

    void release()
    {
        if (initialized && app_ctx) {
            release_yolov6_model((rknn_app_context_t*)app_ctx);
            free(app_ctx);
            app_ctx = nullptr;
            initialized = false;
            spdlog::info("RKNN模型已释放");
        }
    }

    bool runInference(const QImage &inputImage, QImage &outputImage, void *results)
    {
        if (!initialized || !app_ctx) {
            lastError = "RKNN模型未初始化";
            return false;
        }

        // 创建图像缓冲区
        image_buffer_t src_image;
        memset(&src_image, 0, sizeof(image_buffer_t));

        // 转换QImage为RGB888格式
        QImage rgbImage = inputImage.convertToFormat(QImage::Format_RGB888);
        src_image.width = rgbImage.width();
        src_image.height = rgbImage.height();
        src_image.format = IMAGE_FORMAT_RGB888;
        src_image.size = rgbImage.width() * rgbImage.height() * 3;
        src_image.virt_addr = (unsigned char *)malloc(src_image.size);

        if (src_image.virt_addr == nullptr) {
            lastError = "分配图像缓冲区内存失败";
            spdlog::error("{}", lastError.toStdString());
            return false;
        }

        // 复制图像数据
        memcpy(src_image.virt_addr, rgbImage.constBits(), src_image.size);

        // 执行推理
        object_detect_result_list local_results;
        object_detect_result_list *results_ptr = results ? (object_detect_result_list*)results : &local_results;

        int ret = inference_yolov6_model((rknn_app_context_t*)app_ctx, &src_image, results_ptr);
        if (ret != 0) {
            lastError = QString("RKNN推理失败，返回码: %1").arg(ret);
            spdlog::error("{}", lastError.toStdString());
            free(src_image.virt_addr);
            return false;
        }

        spdlog::info("RKNN推理成功，检测到{}个目标", results_ptr->count);

        // 复制原图用于绘制结果
        outputImage = inputImage.copy();

        // 绘制检测框
        if (results_ptr->count > 0) {
            ImageProcessor processor;
            ImageProcessor::DetectResultList detectResults = ImageProcessor::convertResults(results_ptr);
            processor.drawResults(outputImage, detectResults);
        }

        // 释放图像内存
        free(src_image.virt_addr);

        return true;
    }
};

InferenceEngine::InferenceEngine()
    : d_ptr(std::make_unique<InferenceEnginePrivate>())
{
}

InferenceEngine::~InferenceEngine()
{
    // unique_ptr 自动释放
}

bool InferenceEngine::initialize(const QString &modelPath)
{
    return d_ptr->initializeModel(modelPath);
}

void InferenceEngine::release()
{
    d_ptr->release();
}

bool InferenceEngine::isInitialized() const
{
    return d_ptr->initialized;
}

bool InferenceEngine::detect(const QImage &inputImage, QImage &outputImage, void *results)
{
    return d_ptr->runInference(inputImage, outputImage, results);
}

QPair<int, int> InferenceEngine::getInputSize() const
{
    return qMakePair(d_ptr->modelWidth, d_ptr->modelHeight);
}

QString InferenceEngine::getLastError() const
{
    return d_ptr->lastError;
}

void* InferenceEngine::getContext() const
{
    return d_ptr->app_ctx;
}
