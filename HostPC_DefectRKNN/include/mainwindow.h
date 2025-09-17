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
#include <QMediaMetaData>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "camerawindow.h"
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
    void openCamera();
    void toggleVideoInference();
    void processVideoFrame(const QVideoFrame &frame);
    void displayInferenceResult(const QImage &resultImage);
    void showPreviousImage();
    void showNextImage();
    QWidget* createButtonGroup(const QList<QPushButton*> &buttons);
    
private:
    void setupUI();
    void initializeRKNN();
    void loadImage(const QString &path);
    bool runRKNNInference(const QImage &inputImage, QImage &outputImage, object_detect_result_list *od_results = nullptr);
    void displayResult(const QImage &image);
    void processFolder(const QString &folderPath);
    QStringList findImageFiles(const QString &folderPath);
    bool saveResultImage(const QImage &image, const QString &originalPath);
        QImage videoFrameToImage(const QVideoFrame &frame);
    void initVideoInference();
    void startVideoInference();
    void stopVideoInference();
    void updateDefectInfoTable(const object_detect_result_list &od_results);

    // UI组件
    QPushButton *openButton;
    QPushButton *detectButton;
    QPushButton *openFolderButton;
    QPushButton *batchDetectButton;
    QPushButton *prevImageButton;
    QPushButton *nextImageButton;
    QPushButton *openVideoButton;
    QPushButton *inferenceButton;
    QPushButton *openCameraButton;
    QLabel *imageLabel;
    QLabel *statusLabel;
    QLabel *inferenceStatusLabel;
    QTimer *videoTimer;
    QStackedLayout *stackedLayout;
    QLabel *inferenceResultLabel;
    QTableWidget *defectInfoTable;

    // 摄像头窗口
    CameraWindow *cameraWindow;

    // 当前图片路径
    QString currentImagePath;
    // 当前选择的文件夹路径（用于批量检测）
    QString currentFolderPath;
    // 当前视频路径
    QString currentVideoPath;
    // 当前文件夹中的图片列表
    QStringList currentImageList;
    // 当前图片在列表中的索引
    int currentImageIndex;

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

    // 线程同步相关
    QMutex inferenceMutex;
    QQueue<QVideoFrame> frameQueue;
    QWaitCondition frameCondition;

    };

#endif // MAINWINDOW_H