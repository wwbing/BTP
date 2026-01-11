#include "fileservice.h"
#include <QDebug>
#include <spdlog/spdlog.h>

FileService::FileService()
{
}

FileService::~FileService()
{
}

QStringList FileService::findImageFiles(const QString &folderPath) const
{
    QStringList imageFiles;
    QDir dir(folderPath);

    if (!dir.exists()) {
        spdlog::warn("文件夹不存在: {}", folderPath.toStdString());
        return imageFiles;
    }

    // 支持的图片格式
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.tiff" << "*.tif";

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name);  // 按名称排序

    QFileInfoList fileList = dir.entryInfoList();
    for (const QFileInfo &fileInfo : fileList) {
        imageFiles.append(fileInfo.absoluteFilePath());
    }

    spdlog::info("在文件夹中找到 {} 张图片: {}", imageFiles.size(), folderPath.toStdString());
    return imageFiles;
}

QString FileService::saveResultImage(const QImage &image, const QString &originalPath,
                                      const QString &outputDir, const QString &suffix) const
{
    // 确定输出目录
    QString targetDir = outputDir;
    if (targetDir.isEmpty()) {
        QFileInfo info(originalPath);
        targetDir = info.absolutePath();
    }

    // 确保输出目录存在
    if (!ensureDirectoryExists(targetDir)) {
        spdlog::error("无法创建输出目录: {}", targetDir.toStdString());
        return QString();
    }

    // 生成输出路径（使用 targetDir）
    QString outputPath = generateOutputPath(originalPath, suffix, "jpg", targetDir);

    // 保存图像
    if (saveImage(image, outputPath, "JPEG", 90)) {
        spdlog::debug("保存结果图像成功: {}", outputPath.toStdString());
        return outputPath;
    }

    spdlog::error("保存结果图像失败: {}", outputPath.toStdString());
    return QString();
}

bool FileService::saveImage(const QImage &image, const QString &outputPath,
                             const char *format, int quality) const
{
    if (image.isNull()) {
        spdlog::error("无法保存空图像");
        return false;
    }

    // 确保目录存在
    QFileInfo info(outputPath);
    QString dirPath = info.absolutePath();
    if (!ensureDirectoryExists(dirPath)) {
        return false;
    }

    bool success = image.save(outputPath, format, quality);
    if (success) {
        spdlog::info("图像保存成功: {}", outputPath.toStdString());
    } else {
        spdlog::error("图像保存失败: {}", outputPath.toStdString());
    }

    return success;
}

bool FileService::ensureDirectoryExists(const QString &dirPath) const
{
    QDir dir(dirPath);
    if (dir.exists()) {
        return true;
    }

    bool success = dir.mkpath(".");
    if (success) {
        spdlog::info("创建目录成功: {}", dirPath.toStdString());
    } else {
        spdlog::error("创建目录失败: {}", dirPath.toStdString());
    }

    return success;
}

QString FileService::getBaseName(const QString &filePath)
{
    QFileInfo info(filePath);
    return info.baseName();
}

QString FileService::getExtension(const QString &filePath)
{
    QFileInfo info(filePath);
    return info.suffix().toLower();
}

bool FileService::isImageFile(const QString &filePath)
{
    QString ext = getExtension(filePath);
    static const QStringList supportedExts = {"jpg", "jpeg", "png", "bmp", "tiff", "tif"};
    return supportedExts.contains(ext);
}

QString FileService::generateOutputPath(const QString &originalPath,
                                          const QString &suffix,
                                          const QString &newExtension,
                                          const QString &targetDir)
{
    QFileInfo info(originalPath);
    QString baseName = info.baseName();
    QString originalExt = info.suffix();

    // 确定扩展名
    QString ext = newExtension.isEmpty() ? originalExt : newExtension;
    ext = ext.toLower();
    if (!ext.startsWith(".")) {
        ext = "." + ext;
    }

    // 确定输出目录（优先使用 targetDir）
    QString dirPath = targetDir.isEmpty() ? info.absolutePath() : targetDir;
    QString newFileName = baseName + suffix + ext;

    return dirPath + "/" + newFileName;
}

QString FileService::getImageFilters()
{
    return QString("图片文件 (*.png *.jpg *.jpeg *.bmp *.tiff *.tif);;所有文件 (*.*)");
}

QString FileService::getVideoFilters()
{
    return QString("视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv);;所有文件 (*.*)");
}
