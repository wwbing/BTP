#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QProgressDialog>
#include <QDir>
#include <QFileInfo>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QVideoProbe>
#include <QSlider>
#include <QTimer>
#include <QStackedLayout>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QVideoFrame>
// RKNN相关头文件将在cpp文件中包含

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openImage();
    void detectDefects();
    void openFolder();
    void batchDetect();
    void openVideo();
    void playVideo();
    void pauseVideo();
    void stopVideo();
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void setPosition(int position);
    void updateVideoFrame();
    void toggleVideoInference();
    void processVideoFrame(const QVideoFrame &frame);
    void displayInferenceResult(const QImage &resultImage);

private:
    void setupUI();
    QPushButton* createStyledButton(const QString &text, const QString &color);
    QString darkenColor(const QString &color, int percent);
    void initializeRKNN();
    void loadImage(const QString &path);
    bool runRKNNInference(const QImage &inputImage, QImage &outputImage);
    void displayResult(const QImage &image);
    void processFolder(const QString &folderPath);
    QStringList findImageFiles(const QString &folderPath);
    bool saveResultImage(const QImage &image, const QString &originalPath);
    QString formatTime(qint64 milliseconds);
    void updateTimeLabel(qint64 current, qint64 total);
    QImage videoFrameToImage(const QVideoFrame &frame);
    void initVideoInference();
    void startVideoInference();
    void stopVideoInference();

    // UI组件
    QPushButton *openButton;
    QPushButton *detectButton;
    QPushButton *openFolderButton;
    QPushButton *batchDetectButton;
    QPushButton *openVideoButton;
    QPushButton *playButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QPushButton *inferenceButton;
    QLabel *imageLabel;
    QLabel *statusLabel;
    QLabel *inferenceStatusLabel;
    QVideoWidget *videoWidget;
    QSlider *positionSlider;
    QLabel *timeLabel;
    QTimer *videoTimer;
    QStackedLayout *stackedLayout;
    QLabel *inferenceResultLabel;

    // 当前图片路径
    QString currentImagePath;
    // 当前选择的文件夹路径（用于批量检测）
    QString currentFolderPath;
    // 当前视频路径
    QString currentVideoPath;

    // RKNN相关
    void* rknn_app_ctx;  // 使用void*指针避免头文件依赖
    bool rknn_initialized;

    // 视频播放相关
    QMediaPlayer *mediaPlayer;

    // 视频推理相关
    QVideoProbe *videoProbe;
    QThread *inferenceThread;
    bool videoInferenceEnabled;
    bool isProcessingFrame;
    int inferenceFrameCount;
    int totalDetectionCount;
    QMutex inferenceMutex;
    QWaitCondition frameCondition;
    QQueue<QVideoFrame> frameQueue;
    const int MAX_QUEUE_SIZE = 3;
    const int INFERENCE_INTERVAL_MS = 100; // 推理间隔100ms
};

#endif // MAINWINDOW_H