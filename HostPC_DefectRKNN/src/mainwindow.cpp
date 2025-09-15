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
// RKNN相关头文件
#include "rknn_api.h"
#include "yolov6.h"
#include "postprocess.h"
#include "image_utils.h"
#include "file_utils.h"
#include "common.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), rknn_initialized(false), mediaPlayer(nullptr), videoProbe(nullptr), inferenceThread(nullptr), videoInferenceEnabled(false), isProcessingFrame(false), inferenceFrameCount(0), totalDetectionCount(0)
{
    setupUI();
    initializeRKNN();

    // 初始化媒体播放器
    mediaPlayer = new QMediaPlayer(this);
    videoTimer = new QTimer(this);

    // 初始化视频推理相关组件
    initVideoInference();

    
    }

void MainWindow::initVideoInference()
{
    // 创建视频探测器
    videoProbe = new QVideoProbe(this);

    // 连接到媒体播放器
    if (videoProbe->setSource(mediaPlayer)) {
        connect(videoProbe, &QVideoProbe::videoFrameProbed, this, &MainWindow::processVideoFrame);
        qDebug() << "Video probe connected successfully";
    } else {
        qCritical() << "Failed to connect video probe - video inference will not work!";
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
    QHBoxLayout *buttonLayout3 = new QHBoxLayout();

    // 创建按钮
    openButton = new QPushButton("打开图片");
    detectButton = new QPushButton("开始检测");
    openFolderButton = new QPushButton("选择文件夹");
    batchDetectButton = new QPushButton("批量检测");
    openVideoButton = new QPushButton("打开视频");
    inferenceButton = new QPushButton("推理播放");

    // 设置按钮初始状态
    detectButton->setEnabled(false);
    batchDetectButton->setEnabled(false);
    inferenceButton->setEnabled(false);

    // 添加按钮到布局
    buttonLayout1->addWidget(openButton);
    buttonLayout1->addWidget(detectButton);
    buttonLayout2->addWidget(openFolderButton);
    buttonLayout2->addWidget(batchDetectButton);
    buttonLayout3->addWidget(openVideoButton);
    buttonLayout3->addWidget(inferenceButton);

    buttonLayout1->setSpacing(10);
    buttonLayout2->setSpacing(10);
    buttonLayout3->setSpacing(10);

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
    mainLayout->addLayout(buttonLayout3);
    mainLayout->addLayout(stackedLayout, 1);
    mainLayout->addLayout(statusLayout);

    // 连接信号槽
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openImage);
    connect(detectButton, &QPushButton::clicked, this, &MainWindow::detectDefects);
    connect(openFolderButton, &QPushButton::clicked, this, &MainWindow::openFolder);
    connect(batchDetectButton, &QPushButton::clicked, this, &MainWindow::batchDetect);
    connect(openVideoButton, &QPushButton::clicked, this, &MainWindow::openVideo);
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

    qDebug() << "Post process initialized, labels should be loaded";
    
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

    // 验证输入图像，确保是纯视频帧
    qInfo() << "===== 推理前调试信息 =====";
    qInfo() << "送入RKNN的图片分辨率:" << inputImage.size();
    qInfo() << "图片格式:" << inputImage.format();
    qInfo() << "每行字节数:" << inputImage.bytesPerLine();
    qInfo() << "=========================";

    // 直接使用传入的QImage数据
    QImage rgbImage = inputImage.convertToFormat(QImage::Format_RGB888);
    src_image.width = rgbImage.width();
    src_image.height = rgbImage.height();
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.virt_addr = (unsigned char*)malloc(rgbImage.width() * rgbImage.height() * 3);
    src_image.size = rgbImage.width() * rgbImage.height() * 3;

    if (src_image.virt_addr == NULL) {
        qDebug() << "Failed to allocate memory for image buffer";
        return false;
    }

    // 复制QImage数据到图像缓冲区
    memcpy(src_image.virt_addr, rgbImage.constBits(), src_image.size);

    qDebug() << "Created image buffer from QImage:" << src_image.width << "x" << src_image.height;

    // 运行RKNN推理
    object_detect_result_list od_results;
    int ret = inference_yolov6_model((rknn_app_context_t*)rknn_app_ctx, &src_image, &od_results);
    if (ret != 0) {
        qCritical() << "RKNN推理失败，返回码:" << ret;
        // 释放图像内存
        if (src_image.virt_addr != NULL) {
            free(src_image.virt_addr);
        }
        return false;
    }

    qInfo() << "RKNN推理成功，检测到" << od_results.count << "个目标";
    
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

        qInfo() << "Drawing box" << i << "- class:" << coco_cls_to_name(det_result->cls_id)
                << "confidence:" << det_result->prop
                << "coords:(" << x1 << "," << y1 << ")-(" << x2 << "," << y2 << ")";
        
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
                    qInfo() << "视频原始分辨率:" << videoSize;
                }
            });

        // 也在视频加载完成后尝试获取分辨率
        connect(mediaPlayer, static_cast<void(QMediaPlayer::*)(QMediaPlayer::State)>(&QMediaPlayer::stateChanged),
            this, [this](QMediaPlayer::State state) {
            if (state == QMediaPlayer::StoppedState) {
                qInfo() << "视频已加载，状态: LoadedMedia";
                if (mediaPlayer->isVideoAvailable()) {
                    qInfo() << "视频流可用";
                    // 获取视频分辨率
                    QVariant resolution = mediaPlayer->metaData(QMediaMetaData::Resolution);
                    if (resolution.isValid()) {
                        QSize videoSize = resolution.toSize();
                        qInfo() << "视频原始分辨率:" << videoSize;
                    }

                    // 打印更多元数据信息
                    qInfo() << "=== 视频元数据调试信息 ===";
                    QStringList metaDataKeys = mediaPlayer->availableMetaData();
                    for (const QString &key : metaDataKeys) {
                        QVariant value = mediaPlayer->metaData(key);
                        qInfo() << key << ":" << value;
                    }
                    qInfo() << "==============================";

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

    qDebug() << "Video inference started";
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

    qDebug() << "Video inference stopped";
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
        qWarning() << "QVideoProbe failed to convert frame to image";
        return;
    }

    qInfo() << "===== QVideoProbe捕获信息 =====";
    qInfo() << "QVideoProbe捕获的帧分辨率:" << image.size();
    qInfo() << "帧格式:" << image.format();
    qInfo() << "==============================";

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
        qWarning() << "Invalid video frame";
        return QImage();
    }

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        qWarning() << "Failed to map video frame";
        return QImage();
    }

    // 获取帧格式信息
    QVideoFrame::PixelFormat pixelFormat = cloneFrame.pixelFormat();
    QSize size = cloneFrame.size();

    qInfo() << "Converting video frame - format:" << pixelFormat << "size:" << size;

    QImage image;

    // 优先使用QVideoFrame的内置转换功能
    image = cloneFrame.image();

    if (image.isNull()) {
        qWarning() << "QVideoFrame::image() failed, attempting manual conversion";

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
                    qWarning() << "YUV format detected but not fully implemented";
                }
                break;
            default:
                qWarning() << "Unsupported pixel format:" << pixelFormat;
                break;
        }
    }

    cloneFrame.unmap();

    if (image.isNull()) {
        qCritical() << "Failed to convert video frame to image";
        return QImage();
    }

    // 确保图像格式是RKNN支持的RGB888
    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    qInfo() << "Successfully converted video frame to RGB888:" << image.size();
    return image;
}