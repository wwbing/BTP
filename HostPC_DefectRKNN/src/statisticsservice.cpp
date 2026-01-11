#include "statisticsservice.h"
#include "yolov6.h"
#include "postprocess.h"
#include <QDebug>
#include <spdlog/spdlog.h>

StatisticsService::StatisticsService()
    : d_ptr(nullptr)
{
}

StatisticsService::~StatisticsService()
{
}

struct StatisticsService::StatisticsDataPrivate
{
    int totalImages;
    int imagesWithDefects;
    QMap<QString, int> defectCounts;
    QMap<QString, QVector<float>> defectConfidences;
    QMap<QString, int> defectImageCounts;

    StatisticsDataPrivate() : totalImages(0), imagesWithDefects(0) {}
};

void StatisticsService::startNewSession()
{
    if (!d_ptr) {
        d_ptr = std::make_unique<StatisticsDataPrivate>();
    } else {
        d_ptr->totalImages = 0;
        d_ptr->imagesWithDefects = 0;
        d_ptr->defectCounts.clear();
        d_ptr->defectConfidences.clear();
        d_ptr->defectImageCounts.clear();
    }
    spdlog::info("开始新的统计会话");
}

void StatisticsService::collect(void *results, const QString &imagePath)
{
    object_detect_result_list &od_results = *static_cast<object_detect_result_list*>(results);
    if (!d_ptr) {
        d_ptr = std::make_unique<StatisticsDataPrivate>();
    }

    StatisticsDataPrivate &stats = *d_ptr;

    // 增加总图片数
    stats.totalImages++;

    // 如果有检测结果，增加有缺陷的图片数
    if (od_results.count > 0) {
        stats.imagesWithDefects++;
    }

    // 用于跟踪当前图片中已统计的缺陷类型（避免同一类型重复计数）
    QSet<QString> processedTypesInCurrentImage;

    // 统计每种缺陷的数量和置信度
    for (int i = 0; i < od_results.count; i++) {
        const object_detect_result *result = &od_results.results[i];
        char *name = coco_cls_to_name(result->cls_id);
        QString defectType = name ? QString::fromUtf8(name) : QString("unknown");

        // 跳过无效的类别名称
        if (defectType.isEmpty() || defectType == "null") {
            continue;
        }

        // 统计缺陷数量
        stats.defectCounts[defectType]++;

        // 记录置信度
        stats.defectConfidences[defectType].append(result->prop);

        // 统计包含该缺陷类型的图片数（每张图片只统计一次）
        if (!processedTypesInCurrentImage.contains(defectType)) {
            stats.defectImageCounts[defectType]++;
            processedTypesInCurrentImage.insert(defectType);
        }
    }

    spdlog::debug("收集统计数据 - 图片: {}, 缺陷数: {}, 累计缺陷类型: {}",
                  QFileInfo(imagePath).fileName().toStdString(),
                  od_results.count,
                  stats.defectCounts.size());
}

StatisticsService::StatisticsData StatisticsService::getStatistics() const
{
    StatisticsData result;
    if (d_ptr) {
        result.totalImages = d_ptr->totalImages;
        result.imagesWithDefects = d_ptr->imagesWithDefects;
        result.defectCounts = d_ptr->defectCounts;
        result.defectConfidences = d_ptr->defectConfidences;
        result.defectImageCounts = d_ptr->defectImageCounts;
    }
    return result;
}

int StatisticsService::getTotalImages() const
{
    return d_ptr ? d_ptr->totalImages : 0;
}

int StatisticsService::getImagesWithDefects() const
{
    return d_ptr ? d_ptr->imagesWithDefects : 0;
}

int StatisticsService::getTotalDefects() const
{
    if (!d_ptr) return 0;
    int total = 0;
    for (int count : d_ptr->defectCounts) {
        total += count;
    }
    return total;
}

QStringList StatisticsService::getDefectTypes() const
{
    return d_ptr ? d_ptr->defectCounts.keys() : QStringList();
}

int StatisticsService::getDefectCount(const QString &defectType) const
{
    return d_ptr ? d_ptr->defectCounts.value(defectType, 0) : 0;
}

StatisticsService::ConfidenceDistribution StatisticsService::calculateConfidenceDistribution(const QString &defectType) const
{
    ConfidenceDistribution distribution;
    if (!d_ptr) return distribution;

    QVector<float> confidences;
    if (defectType.isEmpty()) {
        for (auto it = d_ptr->defectConfidences.begin(); it != d_ptr->defectConfidences.end(); ++it) {
            confidences.append(it.value());
        }
    } else {
        confidences = d_ptr->defectConfidences.value(defectType);
    }

    for (float confidence : confidences) {
        if (confidence < 0.5) {
            distribution.counts["0.0-0.5"]++;
        } else if (confidence < 0.6) {
            distribution.counts["0.5-0.6"]++;
        } else if (confidence < 0.7) {
            distribution.counts["0.6-0.7"]++;
        } else if (confidence < 0.8) {
            distribution.counts["0.7-0.8"]++;
        } else if (confidence < 0.9) {
            distribution.counts["0.8-0.9"]++;
        } else {
            distribution.counts["0.9-1.0"]++;
        }
    }

    return distribution;
}

double StatisticsService::calculateAverageConfidence(const QString &defectType) const
{
    if (!d_ptr) return 0.0;

    QVector<float> confidences;
    if (defectType.isEmpty()) {
        for (auto it = d_ptr->defectConfidences.begin(); it != d_ptr->defectConfidences.end(); ++it) {
            confidences.append(it.value());
        }
    } else {
        confidences = d_ptr->defectConfidences.value(defectType);
    }

    if (confidences.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (float confidence : confidences) {
        sum += confidence;
    }
    return sum / confidences.size();
}

double StatisticsService::calculateDefectRate() const
{
    if (!d_ptr || d_ptr->totalImages == 0) {
        return 0.0;
    }
    return static_cast<double>(d_ptr->imagesWithDefects) / d_ptr->totalImages;
}

double StatisticsService::calculateDefectRatio(const QString &defectType) const
{
    if (!d_ptr) return 0.0;
    int totalDefects = getTotalDefects();
    if (totalDefects == 0) {
        return 0.0;
    }
    return static_cast<double>(d_ptr->defectCounts.value(defectType, 0)) / totalDefects;
}

QString StatisticsService::getSummary() const
{
    if (!d_ptr) {
        return "暂无统计数据";
    }

    int totalDefects = getTotalDefects();

    QString summary = QString(
        "统计汇总:\n"
        "总图片数: %1\n"
        "有缺陷图片: %2\n"
        "检测到缺陷总数: %3\n"
        "缺陷类型数: %4\n"
        "缺陷检出率: %5%"
    ).arg(d_ptr->totalImages)
     .arg(d_ptr->imagesWithDefects)
     .arg(totalDefects)
     .arg(d_ptr->defectCounts.size())
     .arg(calculateDefectRate() * 100, 0, 'f', 1);

    // 添加各类型统计
    summary += "\n\n各类型缺陷:";
    for (auto it = d_ptr->defectCounts.begin(); it != d_ptr->defectCounts.end(); ++it) {
        summary += QString("\n  %1: %2 个 (平均置信度: %3)")
                          .arg(it.key())
                          .arg(it.value())
                          .arg(calculateAverageConfidence(it.key()), 0, 'f', 3);
    }

    return summary;
}
