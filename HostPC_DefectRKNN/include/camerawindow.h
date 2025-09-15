#ifndef CAMERAWINDOW_H
#define CAMERAWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <linux/videodev2.h>
#include <spdlog/spdlog.h>
// RKNN相关头文件
#include "rknn_api.h"
#include "yolov6.h"
#include "postprocess.h"

class CameraWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CameraWindow(QWidget *parent = nullptr);
    ~CameraWindow();

private:
    void setupUI();
    bool initCamera();
    void captureFrame();
    void closeCamera();
    bool initRKNN();
    bool runInference(const QImage &inputImage, QImage &outputImage);

private slots:
    void onStartStopClicked();
    void onDetectClicked();
    void onTimerTimeout();

private:
    QLabel *cameraView;
    QPushButton *startStopButton;
    QPushButton *detectButton;
    QPushButton *backButton;
    QTimer *captureTimer;

    // V4L2相关
    int cameraFd;
    unsigned char *buffer;
    struct v4l2_buffer bufferInfo;
    int bufferLength;

    // 摄像头参数
    int width;
    int height;
    bool isRunning;
    bool detectEnabled;

    const char* devicePath;

    // RKNN相关
    rknn_app_context_t* rknn_app_ctx;
    bool rknn_initialized;
};

#endif // CAMERAWINDOW_H