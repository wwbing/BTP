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
#include <QMap>
#include <QVector>
#include <QPair>

#include "camerawindow.h"
#include "statisticsdialog.h"
#include "inferenceengine.h"
#include "statisticsservice.h"
#include "fileservice.h"
#include "imageprocessor.h"

// 引入 spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

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
    void showStatistics();
    QWidget* createButtonGroup(const QList<QPushButton*> &buttons);

private:
    void setupUI();
    void initLogger();
    void initializeEngine();
    void loadImage(const QString &path);
    void displayResult(const QImage &image);
    void updateDefectInfoTable(const object_detect_result_list &od_results);

    // 视频相关
    void initVideoInference();
    void startVideoInference();
    void stopVideoInference();
    QImage videoFrameToImage(const QVideoFrame &frame);

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
    QLabel *logoLabel;
    QPushButton *showStatsButton;

    // 摄像头窗口
    CameraWindow *cameraWindow;

    // 当前路径
    QString currentImagePath;
    QString currentFolderPath;
    QString currentVideoPath;
    QStringList currentImageList;
    int currentImageIndex;

    // 服务类
    std::unique_ptr<InferenceEngine> inferenceEngine;
    StatisticsService statisticsService;
    FileService fileService;

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
