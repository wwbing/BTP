#ifndef INFERENCEENGINE_H
#define INFERENCEENGINE_H

#include <QString>
#include <QImage>
#include <QPair>
#include <memory>

/**
 * @brief 推理引擎封装类
 *
 * 封装 RKNN 模型加载、推理、资源管理等核心功能。
 * 使用 RAII 风格管理资源，确保异常安全。
 */
class InferenceEngine
{
public:
    /**
     * @brief 默认构造函数
     */
    InferenceEngine();

    /**
     * @brief 析构函数，自动释放资源
     */
    ~InferenceEngine();

    /**
     * @brief 初始化推理引擎
     * @param modelPath 模型文件路径
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize(const QString &modelPath);

    /**
     * @brief 释放推理引擎资源
     */
    void release();

    /**
     * @brief 检查是否已初始化
     * @return true 已初始化
     * @return false 未初始化
     */
    bool isInitialized() const;

    /**
     * @brief 执行缺陷检测推理
     * @param inputImage 输入图像 (QImage)
     * @param outputImage 输出图像（带检测框标注）
     * @param results 检测结果列表指针（可选）
     * @return true 推理成功
     * @return false 推理失败
     */
    bool detect(const QImage &inputImage, QImage &outputImage, void *results = nullptr);

    /**
     * @brief 获取模型输入尺寸
     * @return 尺寸 (width, height)
     */
    QPair<int, int> getInputSize() const;

    /**
     * @brief 获取最后错误信息
     * @return 错误信息字符串
     */
    QString getLastError() const;

    /**
     * @brief 获取内部上下文指针（用于传递结果）
     * @return 上下文指针
     */
    void* getContext() const;

private:
    struct InferenceEnginePrivate;
    std::unique_ptr<InferenceEnginePrivate> d_ptr;
    Q_DISABLE_COPY(InferenceEngine)
};

#endif // INFERENCEENGINE_H
