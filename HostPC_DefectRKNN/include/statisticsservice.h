#ifndef STATISTICSSERVICE_H
#define STATISTICSSERVICE_H

#include <QMap>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QSet>
#include <memory>

// 包含必要的类型定义（由于 postprocess.h 依赖于 yolov6.h，需要按正确顺序包含）
// 注意：由于头文件包含顺序问题，这里不做前向声明，直接使用 void* 在接口层

/**
 * @brief 统计服务类
 *
 * 封装批量检测统计数据收集、计算和管理功能。
 * 与 UI 解耦，可独立测试。
 */
class StatisticsService
{
public:
    /**
     * @brief 统计数据结构
     */
    struct StatisticsData
    {
        int totalImages;                         // 总图片数
        int imagesWithDefects;                   // 有缺陷的图片数
        QMap<QString, int> defectCounts;        // 各缺陷类型数量
        QMap<QString, QVector<float>> defectConfidences; // 各缺陷类型的置信度列表
        QMap<QString, int> defectImageCounts;   // 包含各缺陷类型的图片数

        StatisticsData() : totalImages(0), imagesWithDefects(0) {}

        void clear() {
            totalImages = 0;
            imagesWithDefects = 0;
            defectCounts.clear();
            defectConfidences.clear();
            defectImageCounts.clear();
        }

        bool isEmpty() const {
            return totalImages == 0;
        }
    };

    /**
     * @brief 置信度分布区间
     */
    struct ConfidenceDistribution
    {
        QMap<QString, int> counts;  // 各区间的数量

        ConfidenceDistribution() {
            counts["0.0-0.5"] = 0;
            counts["0.5-0.6"] = 0;
            counts["0.6-0.7"] = 0;
            counts["0.7-0.8"] = 0;
            counts["0.8-0.9"] = 0;
            counts["0.9-1.0"] = 0;
        }
    };

    /**
     * @brief 默认构造函数
     */
    StatisticsService();

    /**
     * @brief 析构函数
     */
    ~StatisticsService();

    /**
     * @brief 开始新的统计会话（清空旧数据）
     */
    void startNewSession();

    /**
     * @brief 收集单张图片的检测结果
     * @param results 检测结果列表
     * @param imagePath 图片路径（用于日志）
     */
    void collect(void *results, const QString &imagePath = QString());

    /**
     * @brief 获取统计数据
     * @return 统计数据副本
     */
    StatisticsData getStatistics() const;

    /**
     * @brief 获取总图片数
     * @return 总图片数
     */
    int getTotalImages() const;

    /**
     * @brief 获取有缺陷的图片数
     * @return 有缺陷的图片数
     */
    int getImagesWithDefects() const;

    /**
     * @brief 获取缺陷总数
     * @return 缺陷总数
     */
    int getTotalDefects() const;

    /**
     * @brief 获取检测到的缺陷类型列表
     * @return 缺陷类型名称列表
     */
    QStringList getDefectTypes() const;

    /**
     * @brief 获取某类型缺陷的数量
     * @param defectType 缺陷类型名称
     * @return 数量
     */
    int getDefectCount(const QString &defectType) const;

    /**
     * @brief 计算置信度分布
     * @param defectType 缺陷类型（可选，为空则计算全部）
     * @return 置信度分布
     */
    ConfidenceDistribution calculateConfidenceDistribution(const QString &defectType = QString()) const;

    /**
     * @brief 计算平均置信度
     * @param defectType 缺陷类型（可选，为空则计算全部）
     * @return 平均置信度
     */
    double calculateAverageConfidence(const QString &defectType = QString()) const;

    /**
     * @brief 计算缺陷检出率
     * @return 缺陷检出率 (0.0 - 1.0)
     */
    double calculateDefectRate() const;

    /**
     * @brief 计算缺陷占比
     * @param defectType 缺陷类型
     * @return 占比 (0.0 - 1.0)
     */
    double calculateDefectRatio(const QString &defectType) const;

    /**
     * @brief 获取摘要信息
     * @return 摘要字符串
     */
    QString getSummary() const;

private:
    struct StatisticsDataPrivate;
    std::unique_ptr<StatisticsDataPrivate> d_ptr;
};

#endif // STATISTICSSERVICE_H
