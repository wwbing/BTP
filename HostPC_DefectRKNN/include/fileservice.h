#ifndef FILESERVICE_H
#define FILESERVICE_H

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QImage>

/**
 * @brief 文件服务类
 *
 * 封装文件查找、图像保存、路径处理等文件操作功能。
 */
class FileService
{
public:
    /**
     * @brief 默认构造函数
     */
    FileService();

    /**
     * @brief 析构函数
     */
    ~FileService();

    /**
     * @brief 查找文件夹中的所有图片文件
     * @param folderPath 文件夹路径
     * @return 图片文件路径列表
     */
    QStringList findImageFiles(const QString &folderPath) const;

    /**
     * @brief 保存检测结果图像
     * @param image 要保存的图像
     * @param originalPath 原始图像路径（用于生成输出路径）
     * @param outputDir 输出目录（可选，为空则使用原始目录）
     * @param suffix 结果文件后缀（默认: _result）
     * @return 保存后的文件路径，失败返回空字符串
     */
    QString saveResultImage(const QImage &image, const QString &originalPath,
                            const QString &outputDir = QString(),
                            const QString &suffix = "_result") const;

    /**
     * @brief 直接保存图像到指定路径
     * @param image 要保存的图像
     * @param outputPath 输出文件路径
     * @param format 输出格式（默认: JPEG）
     * @param quality 质量 (0-100，默认: 90)
     * @return true 保存成功
     */
    bool saveImage(const QImage &image, const QString &outputPath,
                   const char *format = "JPEG", int quality = 90) const;

    /**
     * @brief 创建输出目录（如果不存在）
     * @param dirPath 目录路径
     * @return true 目录存在或创建成功
     */
    bool ensureDirectoryExists(const QString &dirPath) const;

    /**
     * @brief 获取文件名（不含路径和扩展名）
     * @param filePath 文件路径
     * @return 文件名
     */
    static QString getBaseName(const QString &filePath);

    /**
     * @brief 获取文件扩展名（小写）
     * @param filePath 文件路径
     * @return 扩展名（不含点）
     */
    static QString getExtension(const QString &filePath);

    /**
     * @brief 检查文件是否为支持的图片格式
     * @param filePath 文件路径
     * @return true 是支持的图片格式
     */
    static bool isImageFile(const QString &filePath);

    /**
     * @brief 生成带后缀的输出路径
     * @param originalPath 原始路径
     * @param suffix 后缀
     * @param newExtension 新扩展名（可选，为空则保持原扩展名）
     * @param targetDir 目标目录（可选，为空则使用原始目录）
     * @return 输出路径
     */
    static QString generateOutputPath(const QString &originalPath,
                                       const QString &suffix,
                                       const QString &newExtension = QString(),
                                       const QString &targetDir = QString());

    /**
     * @brief 获取支持的图片格式过滤器字符串
     * @return Qt 文件对话框格式过滤器字符串
     */
    static QString getImageFilters();

    /**
     * @brief 获取支持的视频格式过滤器字符串
     * @return Qt 文件对话框格式过滤器字符串
     */
    static QString getVideoFilters();
};

#endif // FILESERVICE_H
