#include "camerawindow.h"
#include <QPixmap>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>
#include <cstdlib>

CameraWindow::CameraWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 基本初始化
    cameraFd = -1;
    buffer = nullptr;
    width = 1280;
    height = 720;
    isRunning = false;
    devicePath = "/dev/video0";
    bufferLength = 0;

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
    backButton = new QPushButton("返回", this);

    buttonLayout->addWidget(startStopButton);
    buttonLayout->addWidget(backButton);

    mainLayout->addWidget(buttonWidget);

    // 连接信号槽
    connect(startStopButton, &QPushButton::clicked, this, &CameraWindow::onStartStopClicked);
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
        captureTimer->start(33); // 约30fps
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