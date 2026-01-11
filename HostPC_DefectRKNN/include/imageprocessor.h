#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QImage>
#include <QPainter>
#include <QString>
#include <QColor>
#include <QFont>

/**
 * @brief 图像处理服务类
 *
 * 封装图像格式转换、结果绘制、检测框渲染等功能。
 * 提供 QImage 与内部格式之间的转换桥梁。
 */
class ImageProcessor
{
public:
    /**
     * @brief 检测框样式配置
     */
    struct BoxStyle
    {
        QColor borderColor;      // 边框颜色
        QColor textColor;        // 文字颜色
        QColor backgroundColor;  // 背景颜色
        int borderWidth;         // 边框宽度
        int fontSize;            // 字体大小

        BoxStyle() : borderColor(Qt::red), textColor(Qt::white),
                     backgroundColor(Qt::red), borderWidth(3), fontSize(10) {}
    };

    /**
     * @brief 检测结果数据结构（用于传递）
     */
    struct DetectResult
    {
        int classId;
        float confidence;
        int x1, y1, x2, y2;
    };

    /**
     * @brief 检测结果列表
     */
    using DetectResultList = QList<DetectResult>;

    /**
     * @brief 默认构造函数
     */
    ImageProcessor();

    /**
     * @brief 绘制检测结果到图像
     * @param image 输入图像（将被修改）
     * @param results 检测结果列表
     * @param style 检测框样式（可选）
     */
    void drawResults(QImage &image, const DetectResultList &results,
                     const BoxStyle &style = BoxStyle());

    /**
     * @brief 绘制单个检测框
     * @param painter 绘图对象
     * @param result 检测结果
     * @param style 样式配置
     */
    void drawBox(QPainter &painter, const DetectResult &result, const BoxStyle &style);

    /**
     * @brief 获取默认的检测框样式
     * @param classId 类别ID
     * @return 对应的样式
     */
    static BoxStyle getDefaultStyle(int classId);

    /**
     * @brief 转换图像格式为 RGB888
     * @param image 输入图像
     * @return 转换后的图像
     */
    static QImage toRgb888(const QImage &image);

    /**
     * @brief 缩放图像以适应标签大小（保持宽高比）
     * @param image 输入图像
     * @param maxSize 最大尺寸
     * @return 缩放后的图像
     */
    static QImage scaledToFit(const QImage &image, const QSize &maxSize);

    /**
     * @brief 保存图像为 JPEG 格式
     * @param image 输入图像
     * @param path 输出路径
     * @param quality 质量 (0-100)
     * @return true 保存成功
     */
    static bool saveAsJpeg(const QImage &image, const QString &path, int quality = 90);

    /**
     * @brief 获取类别对应的颜色
     * @param classId 类别ID
     * @return 颜色值
     */
    static QColor getClassColor(int classId);

    /**
     * @brief 获取类别名称
     * @param classId 类别ID
     * @return 名称字符串
     */
    static QString getClassName(int classId);

    /**
     * @brief 将 RKNN 结果转换为本地格式
     * @param od_results RKNN 检测结果
     * @return 本地格式的检测结果列表
     */
    static DetectResultList convertResults(void *od_results);
};

#endif // IMAGEPROCESSOR_H
