#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QTabWidget>
#include <QChartView>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    struct DefectStatistics {
        int totalImages;
        int imagesWithDefects;
        QMap<QString, int> defectCounts;
        QMap<QString, QVector<float>> defectConfidences;
        QMap<QString, int> defectImageCounts;
    };

    explicit StatisticsDialog(const DefectStatistics &stats, QWidget *parent = nullptr);
    ~StatisticsDialog();

private:
    void setupUI();
    void createSummaryTab();
    QWidget* createDefectCountChart();
    QWidget* createDefectRatioChart();
    QWidget* createConfidenceDistributionChart();
    void createImageDistributionChart();

    QWidget* createPieChart(const QMap<QString, int> &data, const QString &title);
    QWidget* createBarChart(const QMap<QString, int> &data, const QString &title);
    QWidget* createHistogram(const QMap<QString, QPair<int, int>> &distribution, const QString &defectType);

    DefectStatistics statistics;
    QTabWidget *tabWidget;
    QVBoxLayout *mainLayout;

    // 辅助方法
    QMap<QString, QPair<int, int>> calculateConfidenceDistribution(const QString &defectType) const;
    QMap<QString, QPair<int, int>> calculateTotalConfidenceDistribution(const QVector<float> &allConfidences) const;
    QMap<QString, double> calculateDefectRatios() const;
    double calculateAverageConfidence(const QVector<float> &confidences) const;
};

#endif // STATISTICSDIALOG_H