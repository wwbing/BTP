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
#include <QLineSeries>
#include <QAreaSeries>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDateEdit>
#include <QProgressBar>
#include <QTabWidget>
#include <QLineEdit>
#include <QMenu>
#include <QAction>

QT_CHARTS_USE_NAMESPACE

/**
 * @brief 统计对话框 - 高级统计分析界面
 *
 * 提供完整的缺陷检测统计分析功能，包括：
 * - KPI指标卡片（带趋势指示）
 * - 缺陷分布饼图
 * - 缺陷数量柱状图
 * - 置信度分布图
 * - 雷达图（综合质量评估）
 * - 检测趋势图
 * - 详细数据表格
 * - 数据导出功能
 */
class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 统计数据结构
     */
    struct DefectStatistics {
        int totalImages;                         // 总图片数
        int imagesWithDefects;                   // 有缺陷的图片数
        QMap<QString, int> defectCounts;        // 各缺陷类型数量
        QMap<QString, QVector<float>> defectConfidences; // 各缺陷类型的置信度列表
        QMap<QString, int> defectImageCounts;   // 包含各缺陷类型的图片数
        QDateTime detectionTime;                // 检测时间
        QString sessionName;                    // 检测会话名称
    };

    /**
     * @brief 质量评估结果
     */
    struct QualityAssessment {
        double overallScore;          // 综合质量评分 (0-100)
        double detectionRate;         // 检出率
        double falsePositiveRate;     // 误检率
        double avgConfidence;         // 平均置信度
        QString grade;                // 等级评定 (A/B/C/D)
        QString recommendation;       // 改进建议
    };

    explicit StatisticsDialog(const DefectStatistics &stats, QWidget *parent = nullptr);
    ~StatisticsDialog();

private slots:
    void onExportPDF();
    void onExportCSV();
    void onRefresh();
    void onTabChanged(int index);
    void onSortColumn(int column);
    void onSearchFilter(const QString &text);

private:
    void setupUI();
    void setupToolBar();
    void setupStyle();

    // 顶部区域
    QWidget* createHeader();
    QWidget* createToolBar();

    // KPI区域
    QWidget* createKPISection();

    // 图表区域（选项卡式）
    QWidget* createChartsSection();

    // 质量评估
    QWidget* createQualityAssessment();

    // 详细数据表格
    QWidget* createDetailsSection();

    // 底部摘要
    QWidget* createSummarySection();

    // KPI卡片组件
    QWidget* createKPICard(const QString &title, const QString &value,
                          const QString &unit, const QString &icon,
                          const QColor &color, const QString &trend = "",
                          const QString &trendValue = "");

    // 图表创建
    QChartView* createPieChartView();
    QChartView* createBarChartView();
    QChartView* createConfidenceChartView();
    QChartView* createRadarChartView();
    QChartView* createTrendChartView();
    QChartView* createDefectHeatmap();

    // 辅助方法
    QualityAssessment calculateQualityAssessment();
    double calculateAverageConfidence(const QVector<float> &confidences) const;
    double calculateStdDeviation(const QVector<float> &confidences) const;
    QMap<QString, QPair<int, int>> calculateConfidenceDistribution(const QVector<float> &allConfidences) const;

    // 样式
    void applyModernStyle();
    QString getGradientStyle(const QColor &color) const;
    QString getCardStyle(const QColor &color) const;

    // 数据成员
    DefectStatistics statistics;
    QualityAssessment qualityAssessment;
    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QTabWidget *tabWidget;
    QTableWidget *detailsTable;
    QLineSeries *trendSeries;

    // 排序状态
    int sortColumn = 0;
    bool sortAscending = false;

    // 配色方案
    struct ThemeColors {
        QColor primary;
        QColor success;
        QColor warning;
        QColor danger;
        QColor info;
        QColor purple;
        QColor cyan;
        QList<QColor> chartPalette;
    } colors;

    void initThemeColors();
};

#endif // STATISTICSDIALOG_H
