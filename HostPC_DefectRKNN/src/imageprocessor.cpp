#include "imageprocessor.h"
#include "yolov6.h"
#include "postprocess.h"
#include <QPainter>
#include <QFontMetrics>
#include <QDebug>

ImageProcessor::ImageProcessor()
{
}

void ImageProcessor::drawResults(QImage &image, const DetectResultList &results,
                                  const BoxStyle &style)
{
    if (results.isEmpty()) {
        return;
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Arial", 10));

    for (const DetectResult &result : results) {
        BoxStyle boxStyle = style;

        // 使用默认样式（按类别区分颜色）
        boxStyle = getDefaultStyle(result.classId);

        drawBox(painter, result, boxStyle);
    }

    painter.end();
}

void ImageProcessor::drawBox(QPainter &painter, const DetectResult &result,
                              const BoxStyle &style)
{
    int width = result.x2 - result.x1;
    int height = result.y2 - result.y1;

    // 创建边界框矩形
    QRect rect(result.x1, result.y1, width, height);

    // 绘制边框
    QPen pen(style.borderColor);
    pen.setWidth(style.borderWidth);
    painter.setPen(pen);
    painter.drawRect(rect);

    // 获取类别名称
    QString className = getClassName(result.classId);

    // 准备标签文字
    QString label = QString("%1: %2%").arg(className)
                        .arg(result.confidence * 100, 0, 'f', 1);

    // 计算文字尺寸
    QFontMetrics fm(painter.font());
    QRect textRect = fm.boundingRect(label);
    int textWidth = textRect.width() + 10;
    int textHeight = textRect.height() + 4;

    // 标签背景位置（在检测框上方）
    int labelX = result.x1;
    int labelY = result.y1 - textHeight;
    if (labelY < 0) {
        labelY = result.y1; // 如果上方空间不足，放到框内
    }

    // 确保标签不超出图像边界
    if (labelX + textWidth > painter.device()->width()) {
        labelX = painter.device()->width() - textWidth;
    }

    // 绘制标签背景
    QRect bgRect(labelX, labelY, textWidth, textHeight);
    painter.fillRect(bgRect, style.backgroundColor);

    // 绘制标签文字
    painter.setPen(style.textColor);
    painter.drawText(bgRect, Qt::AlignCenter, label);
}

ImageProcessor::BoxStyle ImageProcessor::getDefaultStyle(int classId)
{
    BoxStyle style;
    style.borderColor = getClassColor(classId);
    style.textColor = Qt::white;
    style.backgroundColor = getClassColor(classId);
    style.borderWidth = 3;
    style.fontSize = 10;
    return style;
}

QImage ImageProcessor::toRgb888(const QImage &image)
{
    if (image.format() == QImage::Format_RGB888) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGB888);
}

QImage ImageProcessor::scaledToFit(const QImage &image, const QSize &maxSize)
{
    if (image.isNull()) {
        return QImage();
    }

    if (maxSize.isEmpty()) {
        return image.copy();
    }

    return image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool ImageProcessor::saveAsJpeg(const QImage &image, const QString &path, int quality)
{
    return image.save(path, "JPEG", quality);
}

QColor ImageProcessor::getClassColor(int classId)
{
    // 根据类别ID返回不同颜色
    // 6个缺陷类别: cr, ic, ps, rs, sc, pc
    switch (classId) {
        case 0: return QColor(255, 0, 0);       // cr - 红色
        case 1: return QColor(0, 255, 0);       // ic - 绿色
        case 2: return QColor(0, 0, 255);       // ps - 蓝色
        case 3: return QColor(255, 255, 0);     // rs - 黄色
        case 4: return QColor(255, 0, 255);     // sc - 紫色
        case 5: return QColor(0, 255, 255);     // pc - 青色
        default: return QColor(128, 128, 128);  // 灰色（未知类别）
    }
}

QString ImageProcessor::getClassName(int classId)
{
    char *name = coco_cls_to_name(classId);
    if (name) {
        return QString::fromUtf8(name);
    }
    return QString("unknown");
}

ImageProcessor::DetectResultList ImageProcessor::convertResults(void *od_results)
{
    DetectResultList results;

    if (!od_results) {
        return results;
    }

    object_detect_result_list *rknnResults = (object_detect_result_list*)od_results;

    for (int i = 0; i < rknnResults->count && i < 128; i++) {
        object_detect_result *r = &rknnResults->results[i];
        DetectResult result;
        result.classId = r->cls_id;
        result.confidence = r->prop;
        result.x1 = r->box.left;
        result.y1 = r->box.top;
        result.x2 = r->box.right;
        result.y2 = r->box.bottom;
        results.append(result);
    }

    return results;
}
