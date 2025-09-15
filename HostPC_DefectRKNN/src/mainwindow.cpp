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
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QVideoProbe>
#include <QSlider>
#include <QTimer>
#include <QUrl>
#include <QStackedLayout>
// RKNN相关头文件
#include "rknn_api.h"
#include "yolov6.h"
#include "postprocess.h"
#include "image_utils.h"
#include "file_utils.h"
#include "common.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), rknn_initialized(false), mediaPlayer(nullptr)
{
    setupUI();
    initializeRKNN();

    // 初始化媒体播放器
    mediaPlayer = new QMediaPlayer(this);
    videoTimer = new QTimer(this);

    // 连接媒体播放器信号
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            playButton->setEnabled(true);
            pauseButton->setEnabled(false);
            stopButton->setEnabled(false);
            statusLabel->setText("视频已停止");
        } else if (state == QMediaPlayer::PlayingState) {
            playButton->setEnabled(false);
            pauseButton->setEnabled(true);
            stopButton->setEnabled(true);
            statusLabel->setText("正在播放视频");
        } else if (state == QMediaPlayer::PausedState) {
            playButton->setEnabled(true);
            pauseButton->setEnabled(false);
            stopButton->setEnabled(true);
            statusLabel->setText("视频已暂停");
        }
    });

    // 连接视频定时器
    connect(videoTimer, &QTimer::timeout, this, &MainWindow::updateVideoFrame);
}

MainWindow::~MainWindow()
{
    if (rknn_initialized) {
        release_yolov6_model((rknn_app_context_t*)rknn_app_ctx);
        free(rknn_app_ctx);
    }

    // 清理媒体播放器
    if (mediaPlayer) {
        mediaPlayer->stop();
        delete mediaPlayer;
    }
}

void MainWindow::setupUI()
{
    // 设置主窗口
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    setMinimumSize(1000, 700);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 创建标题区域
    QWidget *titleWidget = new QWidget();
    QVBoxLayout *titleLayout = new QVBoxLayout(titleWidget);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *titleLabel = new QLabel("RKNN 智能缺陷检测系统");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50; margin: 10px;");

    QLabel *subtitleLabel = new QLabel("基于 Rockchip NPU 的高性能检测平台");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("font-size: 14px; color: #7f8c8d; margin-bottom: 10px;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    // 创建控制面板
    QWidget *controlPanel = new QWidget();
    controlPanel->setObjectName("controlPanel");
    controlPanel->setStyleSheet(
        "#controlPanel {"
        "   background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);"
        "   border-radius: 15px;"
        "   padding: 20px;"
        "}"
    );

    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setSpacing(15);

    // 按钮行布局
    QHBoxLayout *buttonLayout1 = new QHBoxLayout();
    QHBoxLayout *buttonLayout2 = new QHBoxLayout();
    QHBoxLayout *buttonLayout3 = new QHBoxLayout();

    openButton = createStyledButton("📁 打开图片", "#3498db");
    detectButton = createStyledButton("🔍 开始检测", "#e74c3c");
    openFolderButton = createStyledButton("📂 选择文件夹", "#9b59b6");
    batchDetectButton = createStyledButton("⚡ 批量检测", "#f39c12");

    // 视频相关按钮
    openVideoButton = createStyledButton("🎬 打开视频", "#27ae60");
    playButton = createStyledButton("▶️ 播放", "#2ecc71");
    pauseButton = createStyledButton("⏸️ 暂停", "#f39c12");
    stopButton = createStyledButton("⏹️ 停止", "#e74c3c");

    detectButton->setEnabled(false);
    batchDetectButton->setEnabled(false);
    playButton->setEnabled(false);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);

    buttonLayout1->addWidget(openButton);
    buttonLayout1->addWidget(detectButton);
    buttonLayout2->addWidget(openFolderButton);
    buttonLayout2->addWidget(batchDetectButton);
    buttonLayout3->addWidget(openVideoButton);
    buttonLayout3->addWidget(playButton);
    buttonLayout3->addWidget(pauseButton);
    buttonLayout3->addWidget(stopButton);

    buttonLayout1->setSpacing(20);
    buttonLayout2->setSpacing(20);
    buttonLayout3->setSpacing(15);

    controlLayout->addLayout(buttonLayout1);
    controlLayout->addLayout(buttonLayout2);
    controlLayout->addLayout(buttonLayout3);

    // 创建图像显示区域
    QWidget *imageContainer = new QWidget();
    imageContainer->setObjectName("imageContainer");
    imageContainer->setStyleSheet(
        "#imageContainer {"
        "   background: #ffffff;"
        "   border: 2px solid #ecf0f1;"
        "   border-radius: 10px;"
        "   box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);"
        "}"
    );

    QVBoxLayout *imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->setContentsMargins(15, 15, 15, 15);

    // 创建堆叠布局，用于在图片和视频之间切换
    stackedLayout = new QStackedLayout();

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(800, 500);
    imageLabel->setStyleSheet(
        "QLabel {"
        "   border: 2px dashed #bdc3c7;"
        "   border-radius: 8px;"
        "   background: #f8f9fa;"
        "   color: #7f8c8d;"
        "   font-size: 16px;"
        "}"
        "QLabel:!pixmap {"
        "   qproperty-text: '请选择图片文件';"
        "}"
    );

    // 创建视频播放器
    videoWidget = new QVideoWidget(this);
    videoWidget->setMinimumSize(800, 500);
    videoWidget->setStyleSheet(
        "QVideoWidget {"
        "   border: 2px solid #bdc3c7;"
        "   border-radius: 8px;"
        "   background: #000000;"
        "}"
    );

    stackedLayout->addWidget(imageLabel);
    stackedLayout->addWidget(videoWidget);
    imageLayout->addLayout(stackedLayout);

    // 创建视频进度控制区域
    QWidget *progressContainer = new QWidget();
    QHBoxLayout *progressLayout = new QHBoxLayout(progressContainer);
    progressLayout->setContentsMargins(0, 10, 0, 0);

    positionSlider = new QSlider(Qt::Horizontal, this);
    positionSlider->setRange(0, 0);
    positionSlider->setMinimumWidth(400);
    positionSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   border: 1px solid #bbb;"
        "   background: white;"
        "   height: 8px;"
        "   border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #3498db;"
        "   border: 1px solid #5c6bc0;"
        "   width: 18px;"
        "   margin: -5px 0;"
        "   border-radius: 9px;"
        "}"
    );

    timeLabel = new QLabel("00:00 / 00:00", this);
    timeLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");

    progressLayout->addWidget(positionSlider);
    progressLayout->addWidget(timeLabel);
    progressLayout->setStretch(0, 1);

    imageLayout->addWidget(progressContainer);

    // 创建状态栏
    QWidget *statusBar = new QWidget();
    statusBar->setObjectName("statusBar");
    statusBar->setStyleSheet(
        "#statusBar {"
        "   background: linear-gradient(90deg, #34495e 0%, #2c3e50 100%);"
        "   border-radius: 8px;"
        "   padding: 12px 20px;"
        "}"
    );

    QHBoxLayout *statusLayout = new QHBoxLayout(statusBar);

    statusLabel = new QLabel("系统就绪 - 请选择图片文件");
    statusLabel->setStyleSheet(
        "color: #ecf0f1;"
        "font-size: 14px;"
        "font-weight: 500;"
    );
    statusLabel->setAlignment(Qt::AlignLeft);

    // 添加右侧信息
    QLabel *versionLabel = new QLabel("v1.0 | RK3588 多核心");
    versionLabel->setStyleSheet(
        "color: #95a5a6;"
        "font-size: 12px;"
    );
    versionLabel->setAlignment(Qt::AlignRight);

    statusLayout->addWidget(statusLabel, 3);
    statusLayout->addWidget(versionLabel, 1);

    // 添加所有组件到主布局
    mainLayout->addWidget(titleWidget);
    mainLayout->addWidget(controlPanel);
    mainLayout->addWidget(imageContainer, 1);
    mainLayout->addWidget(statusBar);

    // 连接信号槽
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(detectButton, &QPushButton::clicked, this, &MainWindow::detectDefects);
    connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openFolder);
    connect(batchDetectButton, &QPushButton::clicked, this, &MainWindow::batchDetect);

    // 视频相关信号槽连接
    connect(openVideoButton, &QPushButton::clicked, this, &MainWindow::openVideo);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::playVideo);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseVideo);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopVideo);
    connect(positionSlider, &QSlider::sliderMoved, this, &MainWindow::setPosition);

    // 设置窗口属性
    setWindowTitle("RKNN 智能缺陷检测系统");
    resize(1200, 800);

    // 设置窗口样式
    setStyleSheet(
        "QMainWindow {"
        "   background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);"
        "}"
        "QPushButton {"
        "   font-size: 14px;"
        "   font-weight: 600;"
        "}"
    );
}

QPushButton* MainWindow::createStyledButton(const QString &text, const QString &color)
{
    QPushButton *button = new QPushButton(text);
    button->setFixedSize(180, 50);
    button->setStyleSheet(
        QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 8px;"
        "   padding: 12px 20px;"
        "   font-size: 14px;"
        "   font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "   transform: translateY(-2px);"
        "   box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);"
        "}"
        "QPushButton:pressed {"
        "   background-color: %3;"
        "   transform: translateY(0px);"
        "   box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);"
        "}"
        "QPushButton:disabled {"
        "   background-color: #bdc3c7;"
        "   color: #7f8c8d;"
        "}"
        ).arg(color).arg(darkenColor(color, 20)).arg(darkenColor(color, 40))
    );
    return button;
}

QString MainWindow::darkenColor(const QString &color, int percent)
{
    QColor originalColor(color);
    qreal h, s, v;
    originalColor.getHsvF(&h, &s, &v);
    v = qMax(0.0, v - (percent / 100.0));
    QColor newColor;
    newColor.setHsvF(h, s, v);
    return newColor.name();
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
    
    // 初始化RKNN模型
    int ret = init_yolov6_model(model_path, (rknn_app_context_t*)rknn_app_ctx);
    if (ret != 0) {
        QMessageBox::warning(this, "错误", "RKNN模型初始化失败");
        statusLabel->setText("RKNN模型初始化失败");
        rknn_initialized = false;
        return;
    }

    rknn_initialized = true;
    statusLabel->setText("✅ RKNN模型已加载 - 使用3个NPU核心并行处理");
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
    statusLabel->setText(QString("📷 已加载: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::detectDefects()
{
    if (currentImagePath.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择图片文件");
        return;
    }

    statusLabel->setText("🔍 正在检测中，请稍候...");
    QApplication::processEvents();

    // 读取图片
    QImage inputImage(currentImagePath);
    if (inputImage.isNull()) {
        QMessageBox::warning(this, "错误", "无法读取图片文件");
        statusLabel->setText("❌ 检测失败");
        return;
    }

    // 运行RKNN推理
    QImage outputImage;
    if (runRKNNInference(inputImage, outputImage)) {
        displayResult(outputImage);
        statusLabel->setText("✅ 检测完成");
    } else {
        QMessageBox::warning(this, "错误", "RKNN推理失败");
        statusLabel->setText("❌ 检测失败");
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
    
    // 使用read_image函数读取图像（参考rknn_infer实现）
    QByteArray imagePathBytes = currentImagePath.toUtf8();
    int ret = read_image(imagePathBytes.constData(), &src_image);
    if (ret != 0) {
        qDebug() << "读取图像失败";
        return false;
    }
    
    // 运行RKNN推理
    object_detect_result_list od_results;
    ret = inference_yolov6_model((rknn_app_context_t*)rknn_app_ctx, &src_image, &od_results);
    if (ret != 0) {
        qDebug() << "RKNN推理失败";
        // 释放图像内存
        if (src_image.virt_addr != NULL) {
            free(src_image.virt_addr);
        }
        return false;
    }
    
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
        
        // 绘制边界框 - 使用蓝色，参考rknn_infer
        QRect rect(x1, y1, x2 - x1, y2 - y1);
        painter.setPen(QPen(QColor(0, 0, 255), 2)); // 蓝色边框 (COLOR_BLUE)
        painter.drawRect(rect);
        
        // 绘制标签
        QString confidence = QString::number(det_result->prop * 100, 'f', 1) + "%";
        QString label = QString("%1 %2").arg(coco_cls_to_name(det_result->cls_id)).arg(confidence);
        
        QFontMetrics fm(painter.font());
        QRect textRect = fm.boundingRect(label);
        textRect.moveTo(rect.topLeft() - QPoint(0, textRect.height() + 2));
        textRect.setWidth(textRect.width() + 4);
        
        // 填充标签背景 - 使用半透明白色，确保红色文字清晰可见
        painter.fillRect(textRect, QColor(255, 255, 255, 200)); // 半透明白色背景
        
        // 绘制标签文字 - 使用红色，参考rknn_infer
        painter.setPen(QColor(255, 0, 0)); // 红色文字 (COLOR_RED)
        painter.drawText(textRect, Qt::AlignCenter, label);
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

        statusLabel->setText(QString("📂 已选择文件夹: %1 (%2 张图片)").arg(QFileInfo(folderPath).fileName()).arg(imageFiles.size()));
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
            statusLabel->setText("⏹️ 批量检测已取消");
            break;
        }

        QString imagePath = imageFiles[i];
        QFileInfo fileInfo(imagePath);

        // 更新进度
        progressDialog.setValue(i);
        progressDialog.setLabelText(QString("正在处理: %1").arg(fileInfo.fileName()));
        QApplication::processEvents();

        statusLabel->setText(QString("⚡ 正在处理 %1/%2: %3")
                           .arg(i + 1)
                           .arg(imageFiles.size())
                           .arg(fileInfo.fileName()));

        // 处理单张图片
        QImage inputImage(imagePath);
        if (inputImage.isNull()) {
            qDebug() << "无法读取图片:" << imagePath;
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
                qDebug() << "保存结果:" << resultPath;
            } else {
                failCount++;
                qDebug() << "保存失败:" << resultPath;
            }
        } else {
            failCount++;
            qDebug() << "推理失败:" << imagePath;
        }

        // 定期更新界面显示最后处理的结果
        if (i % 5 == 0 || i == imageFiles.size() - 1) {
            displayResult(outputImage);
            QApplication::processEvents();
        }
    }

    progressDialog.setValue(imageFiles.size());

    // 显示最终结果
    QString summary = QString("🎉 批量检测完成！成功: %1, 失败: %2").arg(successCount).arg(failCount);
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

        // 加载视频文件
        mediaPlayer->setMedia(QUrl::fromLocalFile(fileName));
        mediaPlayer->setVideoOutput(videoWidget);

        // 切换到视频显示
        stackedLayout->setCurrentWidget(videoWidget);

        // 启用播放控制按钮
        playButton->setEnabled(true);
        pauseButton->setEnabled(false);
        stopButton->setEnabled(false);

        statusLabel->setText(QString("🎬 已加载视频: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::playVideo()
{
    if (currentVideoPath.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择视频文件");
        return;
    }

    if (mediaPlayer->state() == QMediaPlayer::PausedState) {
        mediaPlayer->play();
    } else {
        mediaPlayer->play();
    }
}

void MainWindow::pauseVideo()
{
    if (mediaPlayer->state() == QMediaPlayer::PlayingState) {
        mediaPlayer->pause();
    }
}

void MainWindow::stopVideo()
{
    mediaPlayer->stop();
    positionSlider->setValue(0);
    updateTimeLabel(0, mediaPlayer->duration());
}

void MainWindow::updatePosition(qint64 position)
{
    positionSlider->setValue(position);
    updateTimeLabel(position, mediaPlayer->duration());
}

void MainWindow::updateDuration(qint64 duration)
{
    positionSlider->setRange(0, duration);
    updateTimeLabel(mediaPlayer->position(), duration);
}

void MainWindow::setPosition(int position)
{
    mediaPlayer->setPosition(position);
}

void MainWindow::updateVideoFrame()
{
    // 这个函数用于更新视频帧，暂时不需要实现
    // 如果需要后续添加视频帧处理功能可以在这里实现
}

void MainWindow::updateTimeLabel(qint64 current, qint64 total)
{
    QString currentTime = formatTime(current);
    QString totalTime = formatTime(total);
    timeLabel->setText(QString("%1 / %2").arg(currentTime).arg(totalTime));
}

QString MainWindow::formatTime(qint64 milliseconds)
{
    if (milliseconds < 0) {
        return "00:00";
    }

    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    qint64 hours = minutes / 60;
    minutes = minutes % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
}