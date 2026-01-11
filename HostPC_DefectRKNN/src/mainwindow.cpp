#include "mainwindow.h"
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

#include "defect_colors.h"
#include "postprocess.h"
#include "imageprocessor.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mediaPlayer(nullptr)
    , videoProbe(nullptr)
    , inferenceThread(nullptr)
    , videoInferenceEnabled(false)
    , isProcessingFrame(false)
    , inferenceFrameCount(0)
    , totalDetectionCount(0)
    , currentImageIndex(-1)
    , cameraWindow(nullptr)
{
    // 初始化日志
    initLogger();

    // 初始化服务类
    inferenceEngine = std::make_unique<InferenceEngine>();

    // 设置UI
    setupUI();

    // 初始化推理引擎
    initializeEngine();

    // 初始化媒体播放器
    mediaPlayer = new QMediaPlayer(this);
    videoTimer = new QTimer(this);

    // 初始化视频推理
    initVideoInference();

    // 初始化摄像头窗口指针
    cameraWindow = nullptr;

    spdlog::info("主窗口初始化完成");
}

void MainWindow::initLogger()
{
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("rknn_defect_detector.log", true);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("rknn_defect_detector", sinks.begin(), sinks.end());

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::debug);

        spdlog::info("RKNN缺陷检测系统启动");
    } catch (const spdlog::spdlog_ex &ex) {
        qDebug() << "日志初始化失败:" << ex.what();
    }
}

MainWindow::~MainWindow()
{
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
        cameraWindow->close();
        cameraWindow = nullptr;
    }

    // 释放推理引擎
    if (inferenceEngine && inferenceEngine->isInitialized()) {
        inferenceEngine->release();
    }

    // 关闭日志
    spdlog::info("RKNN缺陷检测系统关闭");
    spdlog::drop_all();
}

void MainWindow::setupUI()
{
    // 设置主窗口
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    setMinimumSize(800, 600);

    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 创建左侧按钮区域
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // 创建按钮
    openButton = new QPushButton("打开图片");
    detectButton = new QPushButton("开始检测");
    openFolderButton = new QPushButton("选择文件夹");
    batchDetectButton = new QPushButton("批量检测");
    showStatsButton = new QPushButton("查看统计");
    prevImageButton = new QPushButton("上一张");
    nextImageButton = new QPushButton("下一张");
    openVideoButton = new QPushButton("打开视频");
    inferenceButton = new QPushButton("推理播放");
    openCameraButton = new QPushButton("打开摄像头");

    // 设置按钮初始状态
    detectButton->setEnabled(false);
    batchDetectButton->setEnabled(false);
    showStatsButton->setEnabled(false);
    inferenceButton->setEnabled(false);
    prevImageButton->setEnabled(false);
    nextImageButton->setEnabled(false);

    // 设置按钮固定宽度
    QList<QPushButton*> allButtons = {openButton, detectButton, openFolderButton, batchDetectButton,
                                       showStatsButton, prevImageButton, nextImageButton,
                                       openVideoButton, inferenceButton, openCameraButton};
    for (QPushButton *btn : allButtons) {
        btn->setFixedWidth(120);
    }

    // 创建logo组件
    logoLabel = new QLabel(this);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setMinimumSize(120, 120);
    logoLabel->setMaximumSize(120, 120);
    logoLabel->setScaledContents(false);

    // 加载logo图片
    QString logoPath = QCoreApplication::applicationDirPath() + "/../resources/logo.png";
    QPixmap logoPixmap(logoPath);
    if (logoPixmap.isNull()) {
        logoLabel->setText("Company\nLogo");
        logoLabel->setStyleSheet("background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 5px; padding: 10px; font-weight: bold; color: #666;");
    } else {
        QPixmap scaledPixmap = logoPixmap.scaled(logoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        logoLabel->setPixmap(scaledPixmap);
    }

    // 创建功能分组
    QWidget *imageGroup = createButtonGroup({openButton, detectButton});
    QWidget *folderGroup = createButtonGroup({openFolderButton, batchDetectButton, showStatsButton, prevImageButton, nextImageButton});
    QWidget *videoGroup = createButtonGroup({openVideoButton, inferenceButton});
    QWidget *cameraGroup = createButtonGroup({openCameraButton});

    // 添加分组到左侧布局
    leftLayout->addWidget(logoLabel);
    leftLayout->addStretch(1);
    leftLayout->addWidget(imageGroup);
    leftLayout->addStretch(1);
    leftLayout->addWidget(folderGroup);
    leftLayout->addStretch(1);
    leftLayout->addWidget(videoGroup);
    leftLayout->addStretch(1);
    leftLayout->addWidget(cameraGroup);
    leftLayout->addStretch(2);

    // 创建右侧显示区域
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // 创建堆叠布局
    stackedLayout = new QStackedLayout();

    // 创建图片显示标签
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(640, 360);
    imageLabel->setText("请选择图片文件");
    imageLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);

    // 创建推理结果显示标签
    inferenceResultLabel = new QLabel(this);
    inferenceResultLabel->setAlignment(Qt::AlignCenter);
    inferenceResultLabel->setMinimumSize(640, 360);
    inferenceResultLabel->setText("推理结果将在这里显示");
    inferenceResultLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);

    stackedLayout->addWidget(imageLabel);
    stackedLayout->addWidget(inferenceResultLabel);

    // 创建缺陷信息显示表格
    defectInfoTable = new QTableWidget(this);
    defectInfoTable->setColumnCount(4);
    defectInfoTable->setHorizontalHeaderLabels(QStringList() << "缺陷类型" << "置信度" << "位置" << "尺寸");
    defectInfoTable->horizontalHeader()->setStretchLastSection(false);
    defectInfoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    defectInfoTable->setAlternatingRowColors(true);
    defectInfoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    defectInfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    defectInfoTable->setMaximumHeight(150);
    defectInfoTable->setVisible(false);
    defectInfoTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 创建状态栏
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLabel = new QLabel("系统就绪 - 请选择图片文件");
    inferenceStatusLabel = new QLabel("推理: 未启动");
    QLabel *versionLabel = new QLabel("v1.0 | RK3588");

    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(inferenceStatusLabel);
    statusLayout->addWidget(versionLabel);

    // 添加组件到右侧布局
    rightLayout->addLayout(stackedLayout, 1);
    rightLayout->addWidget(defectInfoTable);
    rightLayout->addLayout(statusLayout);

    // 设置左右区域的比例
    mainLayout->addWidget(leftWidget, 1);
    mainLayout->addWidget(rightWidget, 4);

    // 连接信号槽
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(detectButton, &QPushButton::clicked, this, &MainWindow::detectDefects);
    connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openFolder);
    connect(batchDetectButton, &QPushButton::clicked, this, &MainWindow::batchDetect);
    connect(showStatsButton, &QPushButton::clicked, this, &MainWindow::showStatistics);
    connect(prevImageButton, &QPushButton::clicked, this, &MainWindow::showPreviousImage);
    connect(nextImageButton, &QPushButton::clicked, this, &MainWindow::showNextImage);
    connect(openVideoButton, &QPushButton::clicked, this, &MainWindow::openVideo);
    connect(openCameraButton, &QPushButton::clicked, this, &MainWindow::openCamera);
    connect(inferenceButton, &QPushButton::clicked, this, &MainWindow::toggleVideoInference);

    // 设置窗口属性
    setWindowTitle("武汉纺织大学 通用 缺陷检测系统");
    resize(800, 600);
}

QWidget* MainWindow::createButtonGroup(const QList<QPushButton*> &buttons)
{
    QWidget *groupWidget = new QWidget();
    QVBoxLayout *groupLayout = new QVBoxLayout(groupWidget);
    groupLayout->setSpacing(8);
    groupLayout->setContentsMargins(0, 0, 0, 0);

    for (QPushButton *button : buttons) {
        groupLayout->addWidget(button);
    }

    return groupWidget;
}

void MainWindow::initializeEngine()
{
    // 模型路径
    QString appPath = QCoreApplication::applicationDirPath();
    QString modelPath = appPath + "/../model/neu-det-new.rknn";

    if (inferenceEngine->initialize(modelPath)) {
        statusLabel->setText("RKNN模型已加载");
        batchDetectButton->setEnabled(true);
        spdlog::info("推理引擎初始化成功");
    } else {
        QMessageBox::warning(this, "错误", "RKNN模型初始化失败: " + inferenceEngine->getLastError());
        statusLabel->setText("RKNN模型初始化失败");
    }
}

void MainWindow::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择图片文件"),
        "",
        FileService::getImageFilters());

    if (!fileName.isEmpty()) {
        // 清空图片列表和索引
        currentImageList.clear();
        currentImageIndex = -1;
        prevImageButton->setEnabled(false);
        nextImageButton->setEnabled(false);

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
    QSize labelSize = imageLabel->minimumSize();
    if (labelSize.isEmpty()) {
        labelSize = QSize(640, 360);
    }

    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
    detectButton->setEnabled(true);

    // 更新状态栏
    QString statusText;
    if (!currentImageList.isEmpty()) {
        statusText = QString(" 已加载: %1 (%2/%3)")
                         .arg(QFileInfo(path).fileName())
                         .arg(currentImageIndex + 1)
                         .arg(currentImageList.size());
    } else {
        statusText = QString(" 已加载: %1").arg(QFileInfo(path).fileName());
    }
    statusLabel->setText(statusText);

    // 清空缺陷信息表格
    defectInfoTable->setVisible(false);
    defectInfoTable->setRowCount(0);
}

void MainWindow::detectDefects()
{
    if (currentImagePath.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择图片文件");
        return;
    }

    if (!inferenceEngine->isInitialized()) {
        QMessageBox::warning(this, "错误", "推理引擎未初始化");
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

    // 执行推理
    QImage outputImage;
    object_detect_result_list od_results;

    if (inferenceEngine->detect(inputImage, outputImage, &od_results)) {
        displayResult(outputImage);
        updateDefectInfoTable(od_results);
        statusLabel->setText("检测完成");
    } else {
        QMessageBox::warning(this, "错误", "推理失败: " + inferenceEngine->getLastError());
        statusLabel->setText("检测失败");
    }
}

void MainWindow::displayResult(const QImage &image)
{
    QPixmap pixmap = QPixmap::fromImage(image);

    QSize labelSize = imageLabel->minimumSize();
    if (labelSize.isEmpty()) {
        labelSize = QSize(640, 360);
    }

    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
}

void MainWindow::updateDefectInfoTable(const object_detect_result_list &od_results)
{
    defectInfoTable->setRowCount(0);

    if (od_results.count > 0) {
        defectInfoTable->setVisible(true);
        defectInfoTable->setRowCount(od_results.count);

        for (int i = 0; i < od_results.count; i++) {
            const object_detect_result *result = &od_results.results[i];

            // 缺陷类型
            QTableWidgetItem *typeItem = new QTableWidgetItem(ImageProcessor::getClassName(result->cls_id));
            typeItem->setTextAlignment(Qt::AlignCenter);
            defectInfoTable->setItem(i, 0, typeItem);

            // 置信度
            QTableWidgetItem *confItem = new QTableWidgetItem(QString::number(result->prop, 'f', 3));
            confItem->setTextAlignment(Qt::AlignCenter);
            defectInfoTable->setItem(i, 1, confItem);

            // 位置
            QString position = QString("(%1, %2)").arg(result->box.left).arg(result->box.top);
            QTableWidgetItem *posItem = new QTableWidgetItem(position);
            posItem->setTextAlignment(Qt::AlignCenter);
            defectInfoTable->setItem(i, 2, posItem);

            // 尺寸
            QString size = QString("%1 x %2").arg(result->box.right - result->box.left)
                                                .arg(result->box.bottom - result->box.top);
            QTableWidgetItem *sizeItem = new QTableWidgetItem(size);
            sizeItem->setTextAlignment(Qt::AlignCenter);
            defectInfoTable->setItem(i, 3, sizeItem);
        }

        // 设置列宽比例
        int tableWidth = defectInfoTable->width();
        defectInfoTable->setColumnWidth(0, tableWidth * 0.25);
        defectInfoTable->setColumnWidth(1, tableWidth * 0.20);
        defectInfoTable->setColumnWidth(2, tableWidth * 0.25);
        defectInfoTable->setColumnWidth(3, tableWidth * 0.30);
    } else {
        defectInfoTable->setVisible(false);
    }
}

void MainWindow::openFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
        tr("选择包含图片的文件夹"),
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!folderPath.isEmpty()) {
        QStringList imageFiles = fileService.findImageFiles(folderPath);
        if (imageFiles.isEmpty()) {
            QMessageBox::warning(this, "警告", "选定的文件夹中没有找到支持的图片文件");
            return;
        }

        statusLabel->setText(QString(" 已选择文件夹: %1 (%2 张图片)")
                                 .arg(QFileInfo(folderPath).fileName())
                                 .arg(imageFiles.size()));
        currentFolderPath = folderPath;
        currentImageList = imageFiles;
        currentImageIndex = 0;

        // 启用上一张/下一张按钮
        prevImageButton->setEnabled(currentImageList.size() > 1);
        nextImageButton->setEnabled(currentImageList.size() > 1);

        // 显示第一张图片
        loadImage(currentImageList.first());
    }
}

void MainWindow::batchDetect()
{
    if (currentFolderPath.isEmpty() || !QFileInfo(currentFolderPath).isDir()) {
        QMessageBox::warning(this, "错误", "请先选择包含图片的文件夹");
        return;
    }

    if (!inferenceEngine->isInitialized()) {
        QMessageBox::warning(this, "错误", "推理引擎未初始化");
        return;
    }

    QString folderPath = currentFolderPath;
    QStringList imageFiles = fileService.findImageFiles(folderPath);

    if (imageFiles.isEmpty()) {
        QMessageBox::warning(this, "警告", "文件夹中没有找到图片文件");
        return;
    }

    // 开始新的统计会话
    statisticsService.startNewSession();

    // 创建进度对话框
    QProgressDialog progressDialog("正在批量处理图片...", "取消", 0, imageFiles.size(), this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setWindowTitle("批量检测进度");
    progressDialog.setMinimumDuration(0);

    // 在文件夹中创建结果输出目录
    QString outputDir = folderPath + "/results";
    fileService.ensureDirectoryExists(outputDir);

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

        currentImagePath = imagePath;
        QImage outputImage;
        object_detect_result_list od_results;

        if (inferenceEngine->detect(inputImage, outputImage, &od_results)) {
            // 收集统计数据
            statisticsService.collect(&od_results, imagePath);

            // 更新缺陷信息表格
            updateDefectInfoTable(od_results);

            // 保存结果图片
            QString resultPath = fileService.saveResultImage(outputImage, imagePath, outputDir);
            if (!resultPath.isEmpty()) {
                successCount++;
                spdlog::debug("保存结果: {}", resultPath.toStdString());
            } else {
                failCount++;
                spdlog::warn("保存失败: {}", imagePath.toStdString());
            }
        } else {
            failCount++;
            spdlog::warn("推理失败: {}", imagePath.toStdString());
        }

        // 定期更新界面
        if (i % 5 == 0 || i == imageFiles.size() - 1) {
            displayResult(outputImage);
            QApplication::processEvents();
        }
    }

    progressDialog.setValue(imageFiles.size());

    // 启用统计按钮
    showStatsButton->setEnabled(!statisticsService.getStatistics().isEmpty());

    // 显示最终结果
    QString summary = QString(" 批量检测完成！成功: %1, 失败: %2").arg(successCount).arg(failCount);
    statusLabel->setText(summary);

    // 添加统计信息
    QString statsSummary = QString("\n\n统计汇总:\n"
                                   "总图片数: %1\n"
                                   "有缺陷图片: %2\n"
                                   "检测到缺陷总数: %3")
                                  .arg(statisticsService.getTotalImages())
                                  .arg(statisticsService.getImagesWithDefects())
                                  .arg(statisticsService.getTotalDefects());

    QMessageBox::StandardButton reply = QMessageBox::question(this, "批量检测完成",
        summary + statsSummary + "\n\n是否查看详细统计信息?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        showStatistics();
    }
}

void MainWindow::showStatistics()
{
    if (statisticsService.getStatistics().isEmpty()) {
        QMessageBox::information(this, "统计信息", "暂无统计数据，请先进行批量检测。");
        return;
    }

    // 转换统计数据格式
    StatisticsDialog::DefectStatistics stats;
    StatisticsService::StatisticsData data = statisticsService.getStatistics();
    stats.totalImages = data.totalImages;
    stats.imagesWithDefects = data.imagesWithDefects;
    stats.defectCounts = data.defectCounts;
    stats.defectConfidences = data.defectConfidences;
    stats.defectImageCounts = data.defectImageCounts;

    StatisticsDialog dialog(stats, this);
    dialog.exec();
}

// 视频相关功能实现
void MainWindow::openVideo()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择视频文件"),
        "",
        FileService::getVideoFilters());

    if (!fileName.isEmpty()) {
        currentVideoPath = fileName;

        // 停止之前的推理
        stopVideoInference();

        // 加载视频文件
        mediaPlayer->setMedia(QUrl::fromLocalFile(fileName));

        // 切换到推理结果显示界面
        stackedLayout->setCurrentWidget(inferenceResultLabel);

        // 启用推理按钮
        inferenceButton->setEnabled(true);

        // 获取视频分辨率
        connect(mediaPlayer, QOverload<const QString&, const QVariant&>::of(&QMediaPlayer::metaDataChanged),
            this, [this](const QString &key, const QVariant &value) {
                if (key == QMediaMetaData::Resolution) {
                    QSize videoSize = value.toSize();
                    spdlog::info("视频原始分辨率: {}x{}", videoSize.width(), videoSize.height());
                }
            });

        statusLabel->setText(QString(" 已加载视频: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::initVideoInference()
{
    videoProbe = new QVideoProbe(this);

    if (videoProbe->setSource(mediaPlayer)) {
        connect(videoProbe, &QVideoProbe::videoFrameProbed, this, &MainWindow::processVideoFrame);
        spdlog::info("Video probe连接成功");
    } else {
        spdlog::error("Video probe连接失败 - 视频推理功能将无法使用!");
    }
}

void MainWindow::toggleVideoInference()
{
    if (!inferenceEngine->isInitialized()) {
        QMessageBox::warning(this, "错误", "推理引擎未初始化");
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

    inferenceButton->setText("停止播放");
    inferenceStatusLabel->setText("推理: 运行中");
    statusLabel->setText("视频推理已启动");

    mediaPlayer->play();

    spdlog::info("视频推理启动");
}

void MainWindow::stopVideoInference()
{
    videoInferenceEnabled = false;

    mediaPlayer->stop();

    // 清空帧队列
    QMutexLocker locker(&inferenceMutex);
    frameQueue.clear();
    locker.unlock();

    frameCondition.wakeAll();

    inferenceButton->setText("推理播放");
    inferenceStatusLabel->setText(QString("推理: 已停止 (处理%1帧)").arg(inferenceFrameCount));
    statusLabel->setText(QString("视频推理已停止 - 处理%1帧").arg(inferenceFrameCount));

    spdlog::info("视频推理停止");
}

void MainWindow::processVideoFrame(const QVideoFrame &frame)
{
    if (!videoInferenceEnabled || !inferenceEngine->isInitialized()) {
        return;
    }

    if (isProcessingFrame) {
        return; // 跳过
    }

    QImage image = videoFrameToImage(frame);
    if (image.isNull()) {
        spdlog::warn("QVideoProbe无法将帧转换为图像");
        return;
    }

    spdlog::debug("QVideoProbe捕获信息 - 帧分辨率:{}x{}", image.width(), image.height());

    isProcessingFrame = true;

    QImage resultImage;
    object_detect_result_list od_results;

    if (inferenceEngine->detect(image, resultImage, &od_results)) {
        displayInferenceResult(resultImage);
        updateDefectInfoTable(od_results);
        inferenceFrameCount++;
        totalDetectionCount++;

        if (inferenceFrameCount % 10 == 0) {
            inferenceStatusLabel->setText(QString("推理: 运行中 (%1帧)").arg(inferenceFrameCount));
        }
    }

    isProcessingFrame = false;
}

void MainWindow::displayInferenceResult(const QImage &resultImage)
{
    stackedLayout->setCurrentWidget(inferenceResultLabel);

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

    QVideoFrame::PixelFormat pixelFormat = cloneFrame.pixelFormat();
    QSize size = cloneFrame.size();

    spdlog::debug("转换视频帧 - 格式:{}, 大小:{}x{}", static_cast<int>(pixelFormat), size.width(), size.height());

    QImage image = cloneFrame.image();

    if (image.isNull()) {
        // 手动转换
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

    // 确保RGB888格式
    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    return image;
}

void MainWindow::openCamera()
{
    spdlog::info("打开摄像头窗口");

    if (cameraWindow) {
        cameraWindow->close();
        cameraWindow = nullptr;
    }

    cameraWindow = new CameraWindow(this);
    cameraWindow->setAttribute(Qt::WA_DeleteOnClose);

    connect(cameraWindow, &QObject::destroyed, this, [this]() {
        cameraWindow = nullptr;
        spdlog::info("摄像头窗口已销毁");
    });

    cameraWindow->show();
    spdlog::info("摄像头窗口已打开");
}

void MainWindow::showPreviousImage()
{
    if (currentImageList.isEmpty() || currentImageIndex <= 0) {
        return;
    }

    currentImageIndex--;
    loadImage(currentImageList[currentImageIndex]);
}

void MainWindow::showNextImage()
{
    if (currentImageList.isEmpty() || currentImageIndex >= currentImageList.size() - 1) {
        return;
    }

    currentImageIndex++;
    loadImage(currentImageList[currentImageIndex]);
}
