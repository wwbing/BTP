#include "camerawindow.h"
#include "defect_colors.h"
#include <QPixmap>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QString>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 基本初始化
    cameraFd = -1;
    buffer = nullptr;
    width = 1280;
    height = 720;
    isRunning = false;
    detectEnabled = false;
    devicePath = "/dev/video0";
    bufferLength = 0;

    // RKNN相关初始化
    rknn_app_ctx = nullptr;
    rknn_initialized = false;

    // 清零bufferInfo结构体
    memset(&bufferInfo, 0, sizeof(bufferInfo));

    setupUI();

    // 设置初始状态
    cameraView->setText("摄像头预览\n点击'开始预览'启用");
    startStopButton->setEnabled(true);

    spdlog::info("CameraWindow构造完成");
}

CameraWindow::~CameraWindow()
{
    closeCamera();

    // 清理RKNN资源
    if (rknn_app_ctx) {
        if (rknn_app_ctx->input_attrs) {
            free(rknn_app_ctx->input_attrs);
        }
        if (rknn_app_ctx->output_attrs) {
            free(rknn_app_ctx->output_attrs);
        }
        release_yolov6_model(rknn_app_ctx);
        free(rknn_app_ctx);
        rknn_app_ctx = nullptr;
    }

    spdlog::info("CameraWindow析构完成");
}

void CameraWindow::setupUI()
{
    setWindowTitle("摄像头预览");
    resize(800, 600);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 摄像头预览区域
    cameraView = new QLabel(this);
    cameraView->setStyleSheet("background-color: black;");
    cameraView->setMinimumHeight(480);
    cameraView->setAlignment(Qt::AlignCenter);
    cameraView->setText("摄像头预览区域");
    cameraView->setStyleSheet("color: white; background-color: black;");
    mainLayout->addWidget(cameraView);

    // 控制按钮
    QWidget *buttonWidget = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);

    startStopButton = new QPushButton("开始预览", this);
    detectButton = new QPushButton("开始检测", this);
    backButton = new QPushButton("返回", this);

    buttonLayout->addWidget(startStopButton);
    buttonLayout->addWidget(detectButton);
    buttonLayout->addWidget(backButton);

    mainLayout->addWidget(buttonWidget);

    // 连接信号槽
    connect(startStopButton, &QPushButton::clicked, this, &CameraWindow::onStartStopClicked);
    connect(detectButton, &QPushButton::clicked, this, &CameraWindow::onDetectClicked);
    connect(backButton, &QPushButton::clicked, this, &CameraWindow::close);

    // 创建定时器
    captureTimer = new QTimer(this);
    connect(captureTimer, &QTimer::timeout, this, &CameraWindow::onTimerTimeout);
}

bool CameraWindow::initCamera()
{
    spdlog::info("开始初始化摄像头设备: {}", devicePath);

    // 打开设备
    cameraFd = open(devicePath, O_RDWR);
    if (cameraFd < 0) {
        spdlog::error("无法打开摄像头设备: {}", strerror(errno));
        return false;
    }
    spdlog::info("摄像头设备打开成功，文件描述符: {}", cameraFd);

    // 查询设备能力
    struct v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    if (ioctl(cameraFd, VIDIOC_QUERYCAP, &caps) < 0) {
        spdlog::error("查询设备能力失败: {}", strerror(errno));
        ::close(cameraFd);
        return false;
    }

    // 设置图像格式
    spdlog::info("设置图像格式为{}x{} MJPG", width, height);
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(cameraFd, VIDIOC_S_FMT, &format) < 0) {
        spdlog::error("设置图像格式失败: {}", strerror(errno));
        ::close(cameraFd);
        return false;
    }

    width = format.fmt.pix.width;
    height = format.fmt.pix.height;
    spdlog::info("设置图像格式成功: {}x{}", width, height);

    // 请求缓冲区
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cameraFd, VIDIOC_REQBUFS, &req) < 0) {
        spdlog::error("请求缓冲区失败: {}", strerror(errno));
        ::close(cameraFd);
        return false;
    }

    // 查询缓冲区
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(cameraFd, VIDIOC_QUERYBUF, &buf) < 0) {
        spdlog::error("查询缓冲区失败: {}", strerror(errno));
        ::close(cameraFd);
        return false;
    }

    bufferLength = buf.length;
    buffer = (unsigned char*)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cameraFd, buf.m.offset);
    if (buffer == MAP_FAILED) {
        spdlog::error("内存映射失败: {}", strerror(errno));
        ::close(cameraFd);
        return false;
    }

    // 将缓冲区放入队列
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferInfo.memory = V4L2_MEMORY_MMAP;
    bufferInfo.index = 0;

    if (ioctl(cameraFd, VIDIOC_QBUF, &bufferInfo) < 0) {
        spdlog::error("将缓冲区放入队列失败: {}", strerror(errno));
        munmap(buffer, bufferLength);
        ::close(cameraFd);
        return false;
    }

    spdlog::info("摄像头初始化成功");
    return true;
}

void CameraWindow::captureFrame()
{
    if (cameraFd < 0 || !isRunning || !buffer) {
        return;
    }

    // 从队列中取出缓冲区
    memset(&bufferInfo, 0, sizeof(bufferInfo));
    bufferInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferInfo.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cameraFd, VIDIOC_DQBUF, &bufferInfo) < 0) {
        return;
    }

    // 检查缓冲区数据有效性
    if (bufferInfo.bytesused > 0 && bufferInfo.length > 0) {
        // 转换为QImage
        QImage image;

        // 使用MJPG解码
        QByteArray jpegData((const char*)buffer, bufferInfo.bytesused);
        if (image.loadFromData(jpegData, "JPEG")) {
            // 如果检测功能已启用且RKNN已初始化，则执行推理
            if (detectEnabled && rknn_initialized) {
                QImage resultImage;
                if (runInference(image, resultImage)) {
                    image = resultImage; // 使用带检测框的图像
                }
            }

            // 显示图像
            if (!image.isNull()) {
                QPixmap pixmap = QPixmap::fromImage(image);
                if (!pixmap.isNull()) {
                    cameraView->setPixmap(pixmap.scaled(
                        cameraView->size(),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation
                    ));
                }
            }
        }
    }

    // 将缓冲区重新放入队列
    if (ioctl(cameraFd, VIDIOC_QBUF, &bufferInfo) < 0) {
        spdlog::error("将缓冲区重新放入队列失败: {}", strerror(errno));
    }
}

void CameraWindow::closeCamera()
{
    if (isRunning) {
        isRunning = false;
        captureTimer->stop();
    }

    if (cameraFd >= 0) {
        // 停止捕获
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(cameraFd, VIDIOC_STREAMOFF, &type);

        // 解除内存映射
        if (buffer != nullptr) {
            munmap(buffer, bufferLength);
            buffer = nullptr;
        }

        ::close(cameraFd);
        cameraFd = -1;
    }

    spdlog::info("摄像头关闭完成");
}

void CameraWindow::onStartStopClicked()
{
    if (!isRunning) {
        // 开始预览 - 首先初始化摄像头
        if (!initCamera()) {
            cameraView->setText("摄像头初始化失败");
            return;
        }

        // 启动视频流
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(cameraFd, VIDIOC_STREAMON, &type) < 0) {
            spdlog::error("启动视频流失败: {}", strerror(errno));
            return;
        }

        isRunning = true;
        captureTimer->start(16); // 约60fps
        startStopButton->setText("停止预览");
        spdlog::info("开始摄像头预览");
    } else {
        // 停止预览
        isRunning = false;
        captureTimer->stop();

        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(cameraFd, VIDIOC_STREAMOFF, &type);

        startStopButton->setText("开始预览");
        cameraView->setText("摄像头预览\n点击'开始预览'启用");
        spdlog::info("停止摄像头预览");
    }
}

void CameraWindow::onTimerTimeout()
{
    captureFrame();
}

bool CameraWindow::initRKNN()
{
    spdlog::info("开始初始化RKNN模型");

    // 分配RKNN应用上下文
    rknn_app_ctx = (rknn_app_context_t*)malloc(sizeof(rknn_app_context_t));
    if (!rknn_app_ctx) {
        spdlog::error("分配RKNN应用上下文失败");
        return false;
    }
    memset(rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    // 计算模型路径
    char exePath[1024];
    memset(exePath, 0, sizeof(exePath));
    if (readlink("/proc/self/exe", exePath, sizeof(exePath) - 1) == -1) {
        spdlog::error("获取可执行文件路径失败");
        free(rknn_app_ctx);
        rknn_app_ctx = nullptr;
        return false;
    }

    std::string exeDir = exePath;
    size_t lastSlash = exeDir.find_last_of('/');
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash);
    }

    std::string modelPath = exeDir + "/../model/neu-det-new.rknn";

    // 检查模型文件是否存在
    struct stat buffer;
    if (stat(modelPath.c_str(), &buffer) != 0) {
        spdlog::error("模型文件不存在: {}", modelPath);
        free(rknn_app_ctx);
        rknn_app_ctx = nullptr;
        return false;
    }

    spdlog::info("使用模型路径: {}", modelPath);

    // 禁用RGA加速，避免RGA相关问题
    setenv("RGA_DISABLE", "1", 1);
    spdlog::info("已禁用RGA加速，使用CPU处理图像");

    // 初始化后处理模块
    if (init_post_process() != 0) {
        spdlog::error("初始化后处理模块失败");
        free(rknn_app_ctx);
        rknn_app_ctx = nullptr;
        return false;
    }

    // 初始化YOLOv6模型
    if (init_yolov6_model(modelPath.c_str(), rknn_app_ctx) != 0) {
        spdlog::error("初始化YOLOv6模型失败");
        deinit_post_process();
        free(rknn_app_ctx);
        rknn_app_ctx = nullptr;
        return false;
    }

    spdlog::info("RKNN模型初始化成功");
    rknn_initialized = true;
    return true;
}

bool CameraWindow::runInference(const QImage &inputImage, QImage &outputImage)
{
    if (!rknn_initialized || !rknn_app_ctx) {
        return false;
    }

    // 创建图像缓冲区
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));

    // 转换输入图像为RGB888格式
    QImage rgbImage = inputImage.convertToFormat(QImage::Format_RGB888);
    src_image.width = rgbImage.width();
    src_image.height = rgbImage.height();
    src_image.format = IMAGE_FORMAT_RGB888;

    // 直接使用QImage的数据指针，避免额外的内存分配和复制
    src_image.virt_addr = (unsigned char*)rgbImage.bits();

    // 执行推理
    object_detect_result_list detect_result;
    memset(&detect_result, 0, sizeof(object_detect_result_list));

    int ret = inference_yolov6_model(rknn_app_ctx, &src_image, &detect_result);
    if (ret != 0) {
        spdlog::error("YOLOv6推理失败");
        return false;
    }

    // 复制输入图像用于绘制结果
    outputImage = rgbImage.copy();

    // 绘制检测结果
    QPainter painter(&outputImage);
    for (int i = 0; i < detect_result.count; i++) {
        object_detect_result *det_result = &(detect_result.results[i]);

        // 创建边界框
        QRect box(det_result->box.left, det_result->box.top,
                  det_result->box.right - det_result->box.left,
                  det_result->box.bottom - det_result->box.top);

        // 使用颜色管理器绘制带颜色的检测框
        const char* class_name = coco_cls_to_name(det_result->cls_id);
        DefectColorManager::drawDefectBox(painter, det_result->cls_id, box, det_result->prop, class_name);
    }

    return true;
}

void CameraWindow::onDetectClicked()
{
    if (!rknn_initialized) {
        // 初始化RKNN模型
        if (!initRKNN()) {
            cameraView->setText("RKNN模型初始化失败");
            return;
        }
    }

    detectEnabled = !detectEnabled;
    detectButton->setText(detectEnabled ? "停止检测" : "开始检测");
    spdlog::info("检测功能: {}", detectEnabled ? "已启用" : "已禁用");
}