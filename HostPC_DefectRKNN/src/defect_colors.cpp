#include "defect_colors.h"
#include <QPainter>
#include <QRect>
#include <cstring>

// 初始化静态成员
DefectColorConfig DefectColorManager::colorConfigs[6];
bool DefectColorManager::initialized = false;

// 初始化颜色配置
void DefectColorManager::initializeColorConfigs() {
    if (initialized) return;

    // 0: cr - 裂纹 (红色)
    colorConfigs[0].boxColor = QColor(255, 0, 0);      // 红色
    colorConfigs[0].textColor = QColor(255, 255, 255); // 白色文字
    colorConfigs[0].bgColor = QColor(255, 0, 0, 180);  // 红色背景

    // 1: ic - 夹杂 (橙色)
    colorConfigs[1].boxColor = QColor(255, 165, 0);    // 橙色
    colorConfigs[1].textColor = QColor(255, 255, 255); // 白色文字
    colorConfigs[1].bgColor = QColor(255, 165, 0, 180); // 橙色背景

    // 2: ps - 压痕 (黄色)
    colorConfigs[2].boxColor = QColor(255, 255, 0);    // 黄色
    colorConfigs[2].textColor = QColor(0, 0, 0);      // 黑色文字
    colorConfigs[2].bgColor = QColor(255, 255, 0, 180); // 黄色背景

    // 3: rs - 划痕 (绿色)
    colorConfigs[3].boxColor = QColor(0, 255, 0);      // 绿色
    colorConfigs[3].textColor = QColor(0, 0, 0);      // 黑色文字
    colorConfigs[3].bgColor = QColor(0, 255, 0, 180);  // 绿色背景

    // 4: sc - 疤痕 (蓝色)
    colorConfigs[4].boxColor = QColor(0, 0, 255);      // 蓝色
    colorConfigs[4].textColor = QColor(255, 255, 255); // 白色文字
    colorConfigs[4].bgColor = QColor(0, 0, 255, 180);  // 蓝色背景

    // 5: pc - 坑点 (紫色)
    colorConfigs[5].boxColor = QColor(128, 0, 128);    // 紫色
    colorConfigs[5].textColor = QColor(255, 255, 255); // 白色文字
    colorConfigs[5].bgColor = QColor(128, 0, 128, 180); // 紫色背景

    initialized = true;
}

// 获取指定缺陷类别的颜色配置
DefectColorConfig DefectColorManager::getDefectColorConfig(int classId) {
    if (!initialized) {
        initializeColorConfigs();
    }

    if (classId >= 0 && classId < 6) {
        return colorConfigs[classId];
    }

    // 未知类别返回灰色配置
    DefectColorConfig unknownConfig;
    unknownConfig.boxColor = QColor(128, 128, 128);    // 灰色
    unknownConfig.textColor = QColor(255, 255, 255);   // 白色文字
    unknownConfig.bgColor = QColor(128, 128, 128, 180); // 灰色背景
    return unknownConfig;
}

// 获取缺陷类别的名称
const char* DefectColorManager::getDefectTypeName(int classId) {
    switch (classId) {
        case 0: return "cr";    // 裂纹
        case 1: return "ic";    // 夹杂
        case 2: return "ps";    // 压痕
        case 3: return "rs";    // 划痕
        case 4: return "sc";    // 疤痕
        case 5: return "pc";    // 坑点
        default: return "unknown";
    }
}

// 获取缺陷类型枚举
DefectType DefectColorManager::getDefectType(int classId) {
    if (classId >= 0 && classId < 6) {
        return static_cast<DefectType>(classId);
    }
    return DEFECT_UNKNOWN;
}

// 绘制带颜色的检测框和标签
void DefectColorManager::drawDefectBox(QPainter& painter,
                                        int classId,
                                        const QRect& box,
                                        float confidence,
                                        const char* className) {
    // 获取颜色配置
    DefectColorConfig config = getDefectColorConfig(classId);

    // 绘制边界框
    painter.setPen(QPen(config.boxColor, 2));
    painter.drawRect(box);

    // 创建标签文字
    char label[100];
    const char* displayName = className ? className : getDefectTypeName(classId);
    snprintf(label, sizeof(label), "%s %.2f", displayName, confidence);

    // 绘制标签背景
    QRect textRect = box;
    textRect.setHeight(20);
    painter.fillRect(textRect, config.bgColor);

    // 绘制标签文字
    painter.setPen(config.textColor);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
}