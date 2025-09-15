#ifndef DEFECT_COLORS_H
#define DEFECT_COLORS_H

#include <QColor>
#include <QRect>

class QPainter;

// 缺陷类别颜色配置结构体
typedef struct {
    QColor boxColor;    // 边界框颜色
    QColor textColor;   // 文字颜色
    QColor bgColor;     // 背景颜色
} DefectColorConfig;

// 缺陷类别枚举
typedef enum {
    DEFECT_CRACK = 0,      // cr - 裂纹
    DEFECT_INCLUSION = 1,  // ic - 夹杂
    DEFECT_PITMARK = 2,    // ps - 压痕
    DEFECT_SCRATCH = 3,    // rs - 划痕
    DEFECT_SCAR = 4,       // sc - 疤痕
    DEFECT_PIT = 5,         // pc - 坑点
    DEFECT_UNKNOWN = -1     // 未知类别
} DefectType;

// 缺陷颜色管理类
class DefectColorManager {
public:
    // 获取指定缺陷类别的颜色配置
    static DefectColorConfig getDefectColorConfig(int classId);

    // 获取缺陷类别的名称
    static const char* getDefectTypeName(int classId);

    // 获取缺陷类型枚举
    static DefectType getDefectType(int classId);

    // 绘制带颜色的检测框和标签
    static void drawDefectBox(QPainter& painter,
                             int classId,
                             const QRect& box,
                             float confidence,
                             const char* className = nullptr);

private:
    // 初始化颜色配置
    static void initializeColorConfigs();

    // 颜色配置数组
    static DefectColorConfig colorConfigs[6];
    static bool initialized;
};

#endif // DEFECT_COLORS_H