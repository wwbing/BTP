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

private slots:
    void onStartStopClicked();
    void onTimerTimeout();

private:
    QLabel *cameraView;
    QPushButton *startStopButton;
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

    const char* devicePath;
};

#endif // CAMERAWINDOW_H