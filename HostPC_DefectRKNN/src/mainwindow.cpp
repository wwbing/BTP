#include "mainwindow.h"
#include "defect_colors.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QPainter>
#include <QCoreApplication>
#include <QDir>
#include <QColor>
#include <QMediaPlayer>
#include <QVideoProbe>
#include <QSlider>
#include <QTimer>
#include <QUrl>
#include <QStackedLayout>
#include <QDateTime>
// RKNN相关头文件
#include "rknn_api.h"
#include "yolov6.h"
#include "postprocess.h"
#include "image_utils.h"
#include "file_utils.h"
#include "common.h"
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), rknn_initialized(false), mediaPlayer(nullptr), videoProbe(nullptr), inferenceThread(nullptr), videoInferenceEnabled(false), isProcessingFrame(false), inferenceFrameCount(0), totalDetectionCount(0)
{
    // 初始化spdlog日志
    try {
        // 创建同时输出到控制台和文件的logger
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("rknn_defect_detector.log", true);

        std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("rknn_defect_detector", sinks.begin(), sinks.end());

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug); // 设置为debug级别，看到更多信息
        spdlog::flush_on(spdlog::level::debug);  // 遇到debug及以上级别就刷新

        spdlog::info("RKNN缺陷检测系统启动");
    } catch (const spdlog::spdlog_ex &ex) {
        qDebug() << "日志初始化失败:" << ex.what();
    }

  
    setupUI();
    initializeRKNN();

    // 初始化媒体播放器
    mediaPlayer = new QMediaPlayer(this);
    videoTimer = new QTimer(this);

    // 初始化视频推理相关组件
    initVideoInference();

    // 初始化摄像头窗口
    cameraWindow = nullptr;


    }

void MainWindow::initVideoInference()
{
    // 创建视频探测器
    videoProbe = new QVideoProbe(this);

    // 连接到媒体播放器
    if (videoProbe->setSource(mediaPlayer)) {
        connect(videoProbe, &QVideoProbe::videoFrameProbed, this, &MainWindow::processVideoFrame);
        spdlog::info("Video probe连接成功");
    } else {
        spdlog::error("Video probe连接失败 - 视频推理功能将无法使用!");
    }
}

MainWindow::~MainWindow()
{
    if (rknn_initialized) {
        release_yolov6_model((rknn_app_context_t*)rknn_app_ctx);
        free(rknn_app_ctx);
    }

    // 停止视频推理
    stopVideoInference();

    // 清理媒体播放器
    if (mediaPlayer) {
        mediaPlayer->stop();
        delete mediaPlayer;
    }

    // 清理视频探测器
    if (videoProbe) {
        delete videoProbe;
    }

    // 清理摄像头窗口
    if (cameraWindow) {
        delete cameraWindow;
    }

    // 关闭日志
    spdlog::info("RKNN缺陷检测系统关闭");
    spdlog::drop_all(); // 清理所有logger
}

void MainWindow::setupUI()
{
    // 设置主窗口
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    setMinimumSize(800, 600);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 创建按钮布局
    QHBoxLayout *buttonLayout1 = new QHBoxLayout();
    QHBoxLayout *buttonLayout2 = new QHBoxLayout();

    // 创建按钮
    openButton = new QPushButton("打开图片");
    detectButton = new QPushButton("开始检测");
    openFolderButton = new QPushButton("选择文件夹");
    batchDetectButton = new QPushButton("批量检测");
    openVideoButton = new QPushButton("打开视频");
    openCameraButton = new QPushButton("打开摄像头");
    inferenceButton = new QPushButton("推理播放");

    // 设置按钮初始状态
    detectButton->setEnabled(false);
    batchDetectButton->setEnabled(false);
    inferenceButton->setEnabled(false);

    // 添加按钮到布局
    buttonLayout1->addWidget(openButton);
    buttonLayout1->addWidget(detectButton);
    buttonLayout1->addWidget(openFolderButton);
    buttonLayout1->addWidget(batchDetectButton);
    buttonLayout2->addWidget(openVideoButton);
    buttonLayout2->addWidget(openCameraButton);
    buttonLayout2->addWidget(inferenceButton);

    buttonLayout1->setSpacing(10);
    buttonLayout2->setSpacing(10);

    // 创建堆叠布局，用于在图片和视频之间切换
    stackedLayout = new QStackedLayout();

    // 创建图片显示标签
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(640, 480);
    imageLabel->setText("请选择图片文件");
    imageLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);

    // 创建推理结果显示标签
    inferenceResultLabel = new QLabel(this);
    inferenceResultLabel->setAlignment(Qt::AlignCenter);
    inferenceResultLabel->setMinimumSize(640, 480);
    inferenceResultLabel->setText("推理结果将在这里显示");
    inferenceResultLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);

    stackedLayout->addWidget(imageLabel);
    stackedLayout->addWidget(inferenceResultLabel);

    // 创建状态栏
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLabel = new QLabel("系统就绪 - 请选择图片文件");
    inferenceStatusLabel = new QLabel("推理: 未启动");
    QLabel *versionLabel = new QLabel("v1.0 | RK3588");

    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(inferenceStatusLabel);
    statusLayout->addWidget(versionLabel);

    // 添加所有组件到主布局
    mainLayout->addLayout(buttonLayout1);
    mainLayout->addLayout(buttonLayout2);
    mainLayout->addLayout(stackedLayout, 1);
    mainLayout->addLayout(statusLayout);

    // 连接信号槽
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(detectButton, &QPushButton::clicked, this, &MainWindow::detectDefects);
    connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openFolder);
    connect(batchDetectButton, &QPushButton::clicked, this, &MainWindow::batchDetect);
    connect(openVideoButton, &QPushButton::clicked, this, &MainWindow::openVideo);
    connect(openCameraButton, &QPushButton::clicked, this, &MainWindow::openCamera);
    connect(inferenceButton, &QPushButton::clicked, this, &MainWindow::toggleVideoInference);

    // 设置窗口属性
    setWindowTitle("RKNN 缺陷检测系统");
    resize(800, 600);
}


void MainWindow::initializeRKNN()
{
    // 分配内存
    rknn_app_ctx = malloc(sizeof(rknn_app_context_t));
    memset(rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    
    // 模型路径 - 使用相对于可执行文件的路径
    QString appPath = QCoreApplication::applicationDirPath();
    QString modelPath = appPath + "/../model/neu-det-new.rknn";
    const char *model_path = modelPath.toUtf8().constData();
    
    // 设置工作目录到可执行文件所在目录，确保能找到标签文件
    QString originalDir = QDir::currentPath();
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    // 初始化后处理
    init_post_process();

    // 恢复原始工作目录
    QDir::setCurrent(originalDir);

    spdlog::debug("Post process初始化完成，标签已加载");
    
    // 初始化RKNN模型
    int ret = init_yolov6_model(model_path, (rknn_app_context_t*)rknn_app_ctx);
    if (ret != 0) {
        QMessageBox::warning(this, "错误", "RKNN模型初始化失败");
        statusLabel->setText("RKNN模型初始化失败");
        rknn_initialized = false;
        return;
    }

    rknn_initialized = true;
    statusLabel->setText("RKNN模型已加载");
    batchDetectButton->setEnabled(true);
}

void MainWindow::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择图片文件"),
        "",
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.tiff);;所有文件 (*.*)"));

    if (!fileName.isEmpty()) {
        loadImage(fileName);
    }
}

void MainWindow::loadImage(const QString &path)
{
    currentImagePath = path;
    QPixmap pixmap(path);

    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片文件");
        return;
    }

    // 切换到图片显示
    stackedLayout->setCurrentWidget(imageLabel);

    // 缩放图片以适应标签
    QPixmap scaledPixmap = pixmap.scaled(imageLabel->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
    detectButton->setEnabled(true);
    statusLabel->setText(QString(" 已加载: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::detectDefects()
{
    if (currentImagePath.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择图片文件");
        return;
    }

    statusLabel->setText("正在检测中，请稍候...");
    QApplication::processEvents();

    // 读取图片
    QImage inputImage(currentImagePath);
    if (inputImage.isNull()) {
        QMessageBox::warning(this, "错误", "无法读取图片文件");
        statusLabel->setText("检测失败");
        return;
    }

    // 运行RKNN推理
    QImage outputImage;
    if (runRKNNInference(inputImage, outputImage)) {
        displayResult(outputImage);
        statusLabel->setText("检测完成");
    } else {
        QMessageBox::warning(this, "错误", "RKNN推理失败");
        statusLabel->setText("检测失败");
    }
}

bool MainWindow::runRKNNInference(const QImage &inputImage, QImage &outputImage)
{
    if (!rknn_initialized) {
        return false;
    }

    // 创建图像缓冲区
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));

    // 直接使用传入的QImage数据
    QImage rgbImage = inputImage.convertToFormat(QImage::Format_RGB888);
    src_image.width = rgbImage.width();
    src_image.height = rgbImage.height();
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.virt_addr = (unsigned char*)malloc(rgbImage.width() * rgbImage.height() * 3);
    src_image.size = rgbImage.width() * rgbImage.height() * 3;

    if (src_image.virt_addr == NULL) {
        spdlog::error("分配图像缓冲区内存失败");
        return false;
    }

    // 复制QImage数据到图像缓冲区
    memcpy(src_image.virt_addr, rgbImage.constBits(), src_image.size);

    // 运行RKNN推理
    object_detect_result_list od_results;
    int ret = inference_yolov6_model((rknn_app_context_t*)rknn_app_ctx, &src_image, &od_results);
    if (ret != 0) {
        spdlog::error("RKNN推理失败，返回码: {}", ret);
        // 释放图像内存
        if (src_image.virt_addr != NULL) {
            free(src_image.virt_addr);
        }
        return false;
    }

    spdlog::info("RKNN推理成功，检测到{}个目标", od_results.count);
    
    // 复制原图用于绘制结果
    outputImage = inputImage.copy();
    
    // 使用QPainter绘制检测结果
    QPainter painter(&outputImage);
    painter.setFont(QFont("Arial", 10));
    
    // 绘制检测框和标签
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det_result = &(od_results.results[i]);

        // 计算相对于原图的坐标
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

        spdlog::trace("绘制边框{} - 类别:{}, 置信度:{}, 坐标:({},{})-({},{})",
                 i, coco_cls_to_name(det_result->cls_id), det_result->prop,
                 x1, y1, x2, y2);

        // 创建边界框
        QRect rect(x1, y1, x2 - x1, y2 - y1);

        // 使用颜色管理器绘制带颜色的检测框
        DefectColorManager::drawDefectBox(painter, det_result->cls_id, rect, det_result->prop, coco_cls_to_name(det_result->cls_id));
    }
    
    painter.end();
    
    // 释放图像内存
    if (src_image.virt_addr != NULL) {
        free(src_image.virt_addr);
    }
    
    return true;
}

void MainWindow::displayResult(const QImage &image)
{
    // 将QImage转换为QPixmap
    QPixmap pixmap = QPixmap::fromImage(image);

    // 缩放图片以适应标签
    QPixmap scaledPixmap = pixmap.scaled(imageLabel->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
}

void MainWindow::openFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
        tr("选择包含图片的文件夹"),
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!folderPath.isEmpty()) {
        QStringList imageFiles = findImageFiles(folderPath);
        if (imageFiles.isEmpty()) {
            QMessageBox::warning(this, "警告", "选定的文件夹中没有找到支持的图片文件");
            return;
        }

        statusLabel->setText(QString(" 已选择文件夹: %1 (%2 张图片)").arg(QFileInfo(folderPath).fileName()).arg(imageFiles.size()));
        currentFolderPath = folderPath; // 保存文件夹路径

        // 可选：显示文件夹中的第一张图片作为预览
        if (!imageFiles.isEmpty()) {
            loadImage(imageFiles.first());
        }
    }
}

void MainWindow::batchDetect()
{
    if (currentFolderPath.isEmpty() || !QFileInfo(currentFolderPath).isDir()) {
        QMessageBox::warning(this, "错误", "请先选择包含图片的文件夹");
        return;
    }

    QString folderPath = currentFolderPath;
    processFolder(folderPath);
}

QStringList MainWindow::findImageFiles(const QString &folderPath)
{
    QStringList imageFiles;
    QDir dir(folderPath);

    // 支持的图片格式
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.tiff" << "*.tif";

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList fileList = dir.entryInfoList();
    for (const QFileInfo &fileInfo : fileList) {
        imageFiles.append(fileInfo.absoluteFilePath());
    }

    return imageFiles;
}

void MainWindow::processFolder(const QString &folderPath)
{
    QStringList imageFiles = findImageFiles(folderPath);
    if (imageFiles.isEmpty()) {
        QMessageBox::warning(this, "警告", "文件夹中没有找到图片文件");
        return;
    }

    // 创建进度对话框
    QProgressDialog progressDialog("正在批量处理图片...", "取消", 0, imageFiles.size(), this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setWindowTitle("批量检测进度");
    progressDialog.setMinimumDuration(0);

    // 在文件夹中创建结果输出目录
    QDir dir(folderPath);
    QString outputDir = dir.absolutePath() + "/results";
    if (!dir.exists(outputDir)) {
        dir.mkdir(outputDir);
    }

    int successCount = 0;
    int failCount = 0;

    for (int i = 0; i < imageFiles.size(); ++i) {
        // 检查是否取消
        if (progressDialog.wasCanceled()) {
            statusLabel->setText("批量检测已取消");
            break;
        }

        QString imagePath = imageFiles[i];
        QFileInfo fileInfo(imagePath);

        // 更新进度
        progressDialog.setValue(i);
        progressDialog.setLabelText(QString("正在处理: %1").arg(fileInfo.fileName()));
        QApplication::processEvents();

        statusLabel->setText(QString(" 正在处理 %1/%2: %3")
                           .arg(i + 1)
                           .arg(imageFiles.size())
                           .arg(fileInfo.fileName()));

        // 处理单张图片
        QImage inputImage(imagePath);
        if (inputImage.isNull()) {
            spdlog::warn("无法读取图片: {}", imagePath.toStdString());
            failCount++;
            continue;
        }

        currentImagePath = imagePath; // 设置当前图片路径
        QImage outputImage;

        if (runRKNNInference(inputImage, outputImage)) {
            // 保存结果图片
            QString resultPath = outputDir + "/" + fileInfo.completeBaseName() + "_result.jpg";
            if (saveResultImage(outputImage, resultPath)) {
                successCount++;
                spdlog::debug("保存结果: {}", resultPath.toStdString());
            } else {
                failCount++;
                spdlog::warn("保存失败: {}", resultPath.toStdString());
            }
        } else {
            failCount++;
            spdlog::warn("推理失败: {}", imagePath.toStdString());
        }

        // 定期更新界面显示最后处理的结果
        if (i % 5 == 0 || i == imageFiles.size() - 1) {
            displayResult(outputImage);
            QApplication::processEvents();
        }
    }

    progressDialog.setValue(imageFiles.size());

    // 显示最终结果
    QString summary = QString(" 批量检测完成！成功: %1, 失败: %2").arg(successCount).arg(failCount);
    statusLabel->setText(summary);

    QMessageBox::information(this, "批量检测完成", summary + QString("\n结果已保存到: %1").arg(outputDir));
}

bool MainWindow::saveResultImage(const QImage &image, const QString &outputPath)
{
    return image.save(outputPath, "JPEG", 90); // 使用JPEG格式，质量90%
}

// 视频相关功能实现
void MainWindow::openVideo()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择视频文件"),
        "",
        tr("视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;所有文件 (*.*)"));

    if (!fileName.isEmpty()) {
        currentVideoPath = fileName;

        // 停止之前的推理
        stopVideoInference();

        // 加载视频文件
        mediaPlayer->setMedia(QUrl::fromLocalFile(fileName));
        // 不需要设置视频输出到widget，QVideoProbe直接从mediaPlayer获取帧

        // 切换到推理结果显示界面
        stackedLayout->setCurrentWidget(inferenceResultLabel);

        // 启用推理按钮
        inferenceButton->setEnabled(true);

        // 获取并打印视频原始分辨率
        connect(mediaPlayer, QOverload<const QString&, const QVariant&>::of(&QMediaPlayer::metaDataChanged),
            this, [this](const QString &key, const QVariant &value) {
                if (key == QMediaMetaData::Resolution) {
                    QSize videoSize = value.toSize();
                    spdlog::info("视频原始分辨率: {}x{}", videoSize.width(), videoSize.height());
                }
            });

        // 也在视频加载完成后尝试获取分辨率
        connect(mediaPlayer, static_cast<void(QMediaPlayer::*)(QMediaPlayer::State)>(&QMediaPlayer::stateChanged),
            this, [this](QMediaPlayer::State state) {
            if (state == QMediaPlayer::StoppedState) {
                spdlog::info("视频已加载，状态: LoadedMedia");
                if (mediaPlayer->isVideoAvailable()) {
                    spdlog::info("视频流可用");
                    // 获取视频分辨率
                    QVariant resolution = mediaPlayer->metaData(QMediaMetaData::Resolution);
                    if (resolution.isValid()) {
                        QSize videoSize = resolution.toSize();
                        spdlog::info("视频原始分辨率: {}x{}", videoSize.width(), videoSize.height());
                    }

                    // 打印更多元数据信息
                    spdlog::debug("视频元数据调试信息开始");
                    QStringList metaDataKeys = mediaPlayer->availableMetaData();
                    for (const QString &key : metaDataKeys) {
                        QVariant value = mediaPlayer->metaData(key);
                        spdlog::debug("  {}: {}", key.toStdString(), value.toString().toStdString());
                    }
                    spdlog::debug("视频元数据调试信息结束");

                    // QVideoWidget已移除，现在直接从mediaPlayer获取原始帧
                }
            }
        });

        statusLabel->setText(QString(" 已加载视频: %1").arg(QFileInfo(fileName).fileName()));
    }
}




// 视频推理相关功能实现
void MainWindow::toggleVideoInference()
{
    if (!rknn_initialized) {
        QMessageBox::warning(this, "错误", "RKNN模型未初始化");
        return;
    }

    if (currentVideoPath.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择视频文件");
        return;
    }

    if (videoInferenceEnabled) {
        stopVideoInference();
    } else {
        startVideoInference();
    }
}

void MainWindow::startVideoInference()
{
    videoInferenceEnabled = true;
    inferenceFrameCount = 0;
    totalDetectionCount = 0;

    // 更新按钮状态
    inferenceButton->setText("停止播放");
    
    // 更新状态显示
    inferenceStatusLabel->setText("推理: 运行中");
    
    statusLabel->setText("视频推理已启动");

    // 开始播放视频
    mediaPlayer->play();

    spdlog::info("视频推理启动");
}

void MainWindow::stopVideoInference()
{
    videoInferenceEnabled = false;

    // 停止视频播放
    mediaPlayer->stop();

    // 清空帧队列
    QMutexLocker locker(&inferenceMutex);
    frameQueue.clear();
    locker.unlock();

    // 唤醒可能等待的线程
    frameCondition.wakeAll();

    // 更新按钮状态
    inferenceButton->setText("推理播放");
    
    // 更新状态显示
    inferenceStatusLabel->setText(QString("推理: 已停止 (处理%1帧)").arg(inferenceFrameCount));
    
    statusLabel->setText(QString("视频推理已停止 - 处理%1帧").arg(inferenceFrameCount));

    spdlog::info("视频推理停止");
}

void MainWindow::processVideoFrame(const QVideoFrame &frame)
{
    if (!videoInferenceEnabled || !rknn_initialized) {
        return;
    }

    // 检查是否正在处理上一帧
    if (isProcessingFrame) {
        return; // 跳过，等待上一帧处理完成
    }

    // 转换帧为图像进行推理
    QImage image = videoFrameToImage(frame);
    if (image.isNull()) {
        spdlog::warn("QVideoProbe无法将帧转换为图像");
        return;
    }

    spdlog::debug("QVideoProbe捕获信息 - 帧分辨率:{}x{}, 帧格式:{}",
                  image.width(), image.height(), static_cast<int>(image.format()));

    // 设置处理标志
    isProcessingFrame = true;

    // 执行RKNN推理
    QImage resultImage;
    if (runRKNNInference(image, resultImage)) {
        // 显示推理结果
        displayInferenceResult(resultImage);

        // 更新统计信息
        inferenceFrameCount++;
        totalDetectionCount++;

        // 每10帧更新一次状态显示
        if (inferenceFrameCount % 10 == 0) {
            inferenceStatusLabel->setText(QString("推理: 运行中 (%1帧)").arg(inferenceFrameCount));
        }
    }

    // 重置处理标志
    isProcessingFrame = false;
}

void MainWindow::displayInferenceResult(const QImage &resultImage)
{
    // 切换到推理结果显示
    stackedLayout->setCurrentWidget(inferenceResultLabel);

    // 缩放图片以适应标签
    QPixmap pixmap = QPixmap::fromImage(resultImage);
    QPixmap scaledPixmap = pixmap.scaled(inferenceResultLabel->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
    inferenceResultLabel->setPixmap(scaledPixmap);
}

QImage MainWindow::videoFrameToImage(const QVideoFrame &frame)
{
    if (!frame.isValid()) {
        spdlog::warn("无效的视频帧");
        return QImage();
    }

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        spdlog::warn("无法映射视频帧");
        return QImage();
    }

    // 获取帧格式信息
    QVideoFrame::PixelFormat pixelFormat = cloneFrame.pixelFormat();
    QSize size = cloneFrame.size();

    spdlog::debug("转换视频帧 - 格式:{}, 大小:{}x{}",
                  static_cast<int>(pixelFormat), size.width(), size.height());

    QImage image;

    // 优先使用QVideoFrame的内置转换功能
    image = cloneFrame.image();

    if (image.isNull()) {
        spdlog::debug("QVideoFrame::image()失败，尝试手动转换");

        // 手动转换更多格式
        switch (pixelFormat) {
            case QVideoFrame::Format_RGB32:
                image = QImage(cloneFrame.bits(), size.width(), size.height(),
                             cloneFrame.bytesPerLine(), QImage::Format_RGB32);
                break;
            case QVideoFrame::Format_ARGB32:
                image = QImage(cloneFrame.bits(), size.width(), size.height(),
                             cloneFrame.bytesPerLine(), QImage::Format_ARGB32);
                break;
            case QVideoFrame::Format_RGB24:
                image = QImage(cloneFrame.bits(), size.width(), size.height(),
                             cloneFrame.bytesPerLine(), QImage::Format_RGB888);
                break;
            case QVideoFrame::Format_YUV420P:
            case QVideoFrame::Format_YV12:
                // YUV格式需要转换，这里使用QImage的转换能力
                image = QImage(size, QImage::Format_RGB888);
                if (!image.isNull()) {
                    image.fill(Qt::black); // 临时填充，实际应该做YUV到RGB的转换
                    spdlog::warn("检测到YUV格式但未完全实现");
                }
                break;
            default:
                spdlog::warn("不支持的像素格式: {}", static_cast<int>(pixelFormat));
                break;
        }
    }

    cloneFrame.unmap();

    if (image.isNull()) {
        spdlog::error("视频帧转换为图像失败");
        return QImage();
    }

    // 确保图像格式是RKNN支持的RGB888
    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    spdlog::debug("成功将视频帧转换为RGB888: {}x{}", image.width(), image.height());
    return image;
}

void MainWindow::openCamera()
{
    spdlog::info("打开摄像头窗口");

    // 如果摄像头窗口已经存在，先关闭它
    if (cameraWindow) {
        cameraWindow->close();
        delete cameraWindow;
        cameraWindow = nullptr;
    }

    // 创建新的摄像头窗口
    cameraWindow = new CameraWindow(this);
    cameraWindow->show();

    spdlog::info("摄像头窗口已打开");
}