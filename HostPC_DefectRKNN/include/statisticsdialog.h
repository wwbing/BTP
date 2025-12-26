#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QChartView>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>

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
    QWidget* createDashboard();
    QWidget* createKpiCards();
    QWidget* createChartsRow();
    QWidget* createDetailsRow();

    // KPI卡片创建
    QWidget* createKpiCard(const QString &title, const QString &value, const QString &unit,
                           const QString &icon, const QColor &accentColor);

    // 图表创建
    QWidget* createChartCard(const QString &title, QWidget *chartWidget);
    QWidget* createDefectPieChart();
    QWidget* createDefectBarChart();
    QWidget* createConfidenceChart();
    QWidget* createDefectDetailsTable();
    QWidget* createEmptyWidget(const QString &message);

    // 辅助方法
    QMap<QString, QPair<int, int>> calculateTotalConfidenceDistribution(const QVector<float> &allConfidences) const;
    QMap<QString, double> calculateDefectRatios() const;
    double calculateAverageConfidence(const QVector<float> &confidences) const;

    // 样式辅助
    QString getCardStyle(const QColor &accentColor) const;
    QString getSectionTitleStyle() const;

    DefectStatistics statistics;
    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
};

#endif // STATISTICSDIALOG_H