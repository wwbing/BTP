#include "statisticsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QDebug>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QPolarChart>
#include <QtCharts/QCategoryAxis>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsLayout>
#include <QFileDialog>
#include <QTextDocument>
#include <QPainter>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QBarSeries>
#include <QCategoryAxis>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QDate>
#include <cmath>
#include <QtMath>

// ============== 配色方案定义 ==============
namespace ChartColors {
    // 现代渐变配色
    const QColor PRIMARY = QColor(99, 102, 241);      // 靛蓝
    const QColor SUCCESS = QColor(34, 197, 94);       // 绿色
    const QColor WARNING = QColor(251, 191, 36);      // 琥珀色
    const QColor DANGER = QColor(239, 68, 68);        // 红色
    const QColor INFO = QColor(59, 130, 246);         // 蓝色
    const QColor PURPLE = QColor(168, 85, 247);       // 紫色
    const QColor CYAN = QColor(6, 182, 212);          // 青色

    // 图表调色板 - 柔和渐变色
    const QList<QColor> PALETTE = {
        QColor(99, 102, 241),  // 靛蓝
        QColor(34, 197, 94),   // 绿色
        QColor(251, 191, 36),  // 琥珀色
        QColor(239, 68, 68),   // 红色
        QColor(168, 85, 247),  // 紫色
        QColor(6, 182, 212),   // 青色
        QColor(249, 115, 22),  // 橙色
        QColor(236, 72, 153),  // 粉色
    };

    // 缺陷类型对应的颜色映射
    const QMap<QString, QColor> DEFECT_COLORS = {
        {"cr", QColor(239, 68, 68)},    // 裂纹 - 红色
        {"ic", QColor(251, 146, 60)},   // 夹杂 - 橙色
        {"ps", QColor(251, 191, 36)},   // 压痕 - 黄色
        {"rs", QColor(34, 197, 94)},    // 划痕 - 绿色
        {"sc", QColor(59, 130, 246)},   // 疤痕 - 蓝色
        {"pc", QColor(168, 85, 247)},   // 坑点 - 紫色
    };
}

// ============== 构造函数和析构函数 ==============
StatisticsDialog::StatisticsDialog(const DefectStatistics &stats, QWidget *parent)
    : QDialog(parent)
    , statistics(stats)
    , tabWidget(nullptr)
    , detailsTable(nullptr)
    , trendSeries(nullptr)
{
    // 初始化配色方案
    initThemeColors();

    // 计算质量评估
    qualityAssessment = calculateQualityAssessment();

    // 设置窗口属性
    setWindowTitle("缺陷检测统计分析报告");
    setMinimumSize(1400, 900);
    resize(1600, 1000);

    setupUI();
}

StatisticsDialog::~StatisticsDialog()
{
}

// ============== 初始化方法 ==============
void StatisticsDialog::initThemeColors()
{
    colors.primary = ChartColors::PRIMARY;
    colors.success = ChartColors::SUCCESS;
    colors.warning = ChartColors::WARNING;
    colors.danger = ChartColors::DANGER;
    colors.info = ChartColors::INFO;
    colors.purple = ChartColors::PURPLE;
    colors.cyan = ChartColors::CYAN;
    colors.chartPalette = ChartColors::PALETTE;
}

void StatisticsDialog::setupUI()
{
    // 设置主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 应用现代样式
    applyModernStyle();

    // 创建标题栏
    mainLayout->addWidget(createHeader());

    // 创建工具栏
    mainLayout->addWidget(createToolBar());

    // 创建主内容区域（可滚动）
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: #f0f2f5; border: none; }");

    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(24, 20, 24, 20);
    contentLayout->setSpacing(20);

    // KPI区域
    contentLayout->addWidget(createKPISection());

    // 选项卡式图表区域
    contentLayout->addWidget(createChartsSection());

    // 质量评估
    contentLayout->addWidget(createQualityAssessment());

    // 详细数据表格
    contentLayout->addWidget(createDetailsSection());

    // 底部摘要
    contentLayout->addWidget(createSummarySection());

    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void StatisticsDialog::applyModernStyle()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #f0f2f5;
        }
        QScrollArea {
            background-color: #f0f2f5;
        }
        QWidget#contentWidget {
            background-color: #f0f2f5;
        }
        QPushButton {
            background-color: #6366f1;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #4f46e5;
        }
        QPushButton:pressed {
            background-color: #4338ca;
        }
        QPushButton.secondary {
            background-color: white;
            color: #374151;
            border: 1px solid #d1d5db;
        }
        QPushButton.secondary:hover {
            background-color: #f3f4f6;
        }
        QTabWidget::pane {
            background-color: white;
            border-radius: 8px;
            border: 1px solid #e5e7eb;
        }
        QTabBar::tab {
            background-color: #f3f4f6;
            color: #6b7280;
            padding: 10px 20px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: 500;
        }
        QTabBar::tab:selected {
            background-color: white;
            color: #6366f1;
        }
        QTableWidget {
            background-color: white;
            border-radius: 8px;
            border: 1px solid #e5e7eb;
            gridline-color: #f3f4f6;
        }
        QHeaderView::section {
            background-color: #f9fafb;
            color: #374151;
            padding: 12px 16px;
            font-weight: 600;
            border-bottom: 1px solid #e5e7eb;
        }
    )");
}

QString StatisticsDialog::getCardStyle(const QColor &color) const
{
    return QString(R"(
        QFrame {
            background-color: white;
            border-radius: 12px;
            border: 1px solid #e5e7eb;
        }
    )");
}

QString StatisticsDialog::getGradientStyle(const QColor &color) const
{
    return QString(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 rgba(%1, %2, %3, 15), "
        "stop:1 rgba(%1, %2, %3, 5));"
    ).arg(color.red()).arg(color.green()).arg(color.blue());
}

// ============== 头部和工具栏 ==============
QWidget* StatisticsDialog::createHeader()
{
    QWidget *header = new QWidget();
    header->setObjectName("header");
    header->setStyleSheet(R"(
        QWidget#header {
            background-color: white;
            border-bottom: 1px solid #e5e7eb;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(24, 16, 24, 16);

    // 标题区域
    QVBoxLayout *titleLayout = new QVBoxLayout();

    QLabel *titleLabel = new QLabel("缺陷检测统计分析报告");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setWeight(QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #111827;");

    QLabel *subtitleLabel = new QLabel(
        QString("检测时间: %1 | 会话: %2")
            .arg(statistics.detectionTime.isValid() ?
                 statistics.detectionTime.toString("yyyy-MM-dd hh:mm:ss") :
                 QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(statistics.sessionName.isEmpty() ? "批量检测" : statistics.sessionName)
    );
    subtitleLabel->setStyleSheet("color: #6b7280; font-size: 13px; margin-top: 4px;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    layout->addLayout(titleLayout);

    layout->addStretch();

    // 质量等级徽章
    QLabel *gradeBadge = new QLabel(QString("质量等级: %1").arg(qualityAssessment.grade));
    gradeBadge->setStyleSheet(QString(R"(
        QLabel {
            background-color: %1;
            color: white;
            padding: 8px 20px;
            border-radius: 20px;
            font-weight: bold;
            font-size: 14px;
        }
    )").arg(qualityAssessment.overallScore >= 90 ? "#22c55e" :
            qualityAssessment.overallScore >= 70 ? "#f59e0b" :
            qualityAssessment.overallScore >= 50 ? "#6366f1" : "#ef4444"));

    layout->addWidget(gradeBadge);

    return header;
}

QWidget* StatisticsDialog::createToolBar()
{
    QWidget *toolbar = new QWidget();
    toolbar->setStyleSheet(R"(
        QWidget {
            background-color: white;
            border-bottom: 1px solid #e5e7eb;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(24, 12, 24, 12);
    layout->setSpacing(16);

    // 搜索框
    QLineEdit *searchBox = new QLineEdit();
    searchBox->setPlaceholderText("搜索缺陷类型...");
    searchBox->setStyleSheet(R"(
        QLineEdit {
            padding: 8px 12px;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            background-color: #f9fafb;
            min-width: 200px;
        }
        QLineEdit:focus {
            border-color: #6366f1;
            outline: none;
        }
    )");
    connect(searchBox, &QLineEdit::textChanged, this, &StatisticsDialog::onSearchFilter);
    layout->addWidget(searchBox);

    layout->addStretch();

    // 刷新按钮
    QPushButton *refreshBtn = new QPushButton("刷新");
    refreshBtn->setIcon(QIcon::fromTheme("view-refresh"));
    refreshBtn->setProperty("class", "secondary");
    connect(refreshBtn, &QPushButton::clicked, this, &StatisticsDialog::onRefresh);
    layout->addWidget(refreshBtn);

    // 导出按钮
    QPushButton *exportBtn = new QPushButton("导出报告");
    exportBtn->setIcon(QIcon::fromTheme("document-export"));
    QMenu *exportMenu = new QMenu(exportBtn);
    exportMenu->addAction("导出 PDF", this, &StatisticsDialog::onExportPDF);
    exportMenu->addAction("导出 CSV", this, &StatisticsDialog::onExportCSV);
    exportBtn->setMenu(exportMenu);
    layout->addWidget(exportBtn);

    // 关闭按钮
    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setProperty("class", "secondary");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    layout->addWidget(closeBtn);

    return toolbar;
}

// ============== KPI 区域 ==============
QWidget* StatisticsDialog::createKPISection()
{
    QWidget *section = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(section);
    layout->setSpacing(16);

    // 计算统计数据
    int totalDefects = 0;
    QVector<float> allConfidences;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        totalDefects += it.value();
        allConfidences.append(statistics.defectConfidences.value(it.key()));
    }

    double defectRate = statistics.totalImages > 0 ?
                        (double)statistics.imagesWithDefects / statistics.totalImages * 100 : 0;
    double avgConfidence = calculateAverageConfidence(allConfidences);
    double passRate = 100.0 - defectRate;

    // 创建KPI卡片
    layout->addWidget(createKPICard(
        "总检测图片",
        QString::number(statistics.totalImages),
        "张",
        "📊",
        colors.info,
        "↗",
        "+12%"
    ), 1);

    layout->addWidget(createKPICard(
        "合格图片",
        QString::number(statistics.totalImages - statistics.imagesWithDefects),
        "张",
        "✅",
        colors.success,
        "↗",
        "+8%"
    ), 1);

    layout->addWidget(createKPICard(
        "缺陷图片",
        QString::number(statistics.imagesWithDefects),
        "张",
        "⚠️",
        colors.warning,
        "↘",
        "-5%"
    ), 1);

    layout->addWidget(createKPICard(
        "合格率",
        QString::number(passRate, 'f', 1),
        "%",
        "📈",
        colors.success,
        "→",
        "稳定"
    ), 1);

    layout->addWidget(createKPICard(
        "缺陷总数",
        QString::number(totalDefects),
        "个",
        "🔍",
        colors.danger
    ), 1);

    layout->addWidget(createKPICard(
        "平均置信度",
        QString::number(avgConfidence, 'f', 3),
        "",
        "🎯",
        colors.primary
    ), 1);

    return section;
}

QWidget* StatisticsDialog::createKPICard(const QString &title, const QString &value,
                                         const QString &unit, const QString &icon,
                                         const QColor &color, const QString &trend,
                                         const QString &trendValue)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(getCardStyle(color));
    card->setProperty("gradient", getGradientStyle(color));

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 16, 20, 16);
    cardLayout->setSpacing(12);

    // 顶部：图标和标题
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    QLabel *iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet(QString(
        "font-size: 24px; "
        "background-color: rgba(%1, %2, %3, 20); "
        "padding: 10px; "
        "border-radius: 10px;"
    ).arg(color.red()).arg(color.green()).arg(color.blue()));

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("color: #6b7280; font-size: 13px;");

    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel, 1);
    cardLayout->addLayout(headerLayout);

    // 中间：数值
    QHBoxLayout *valueLayout = new QHBoxLayout();
    valueLayout->setSpacing(6);

    QLabel *valueLabel = new QLabel(value);
    valueLabel->setStyleSheet(QString(
        "color: #111827; font-size: 32px; font-weight: 700;"
    ));

    QLabel *unitLabel = new QLabel(unit);
    unitLabel->setStyleSheet("color: #9ca3af; font-size: 14px; padding-bottom: 4px;");
    unitLabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

    valueLayout->addWidget(valueLabel);
    valueLayout->addWidget(unitLabel);
    valueLayout->addStretch();
    cardLayout->addLayout(valueLayout);

    // 底部：趋势指示
    if (!trend.isEmpty()) {
        QHBoxLayout *trendLayout = new QHBoxLayout();

        QLabel *trendIcon = new QLabel(trend);
        trendIcon->setStyleSheet(QString(
            "font-size: 14px; "
            "color: %1;"
        ).arg(trend.startsWith("↗") || trend.startsWith("+") ? "#22c55e" :
              trend.startsWith("↘") || trend.startsWith("-") ? "#ef4444" : "#6b7280"));

        QLabel *trendValueLabel = new QLabel(trendValue);
        trendValueLabel->setStyleSheet(QString(
            "font-size: 12px; color: %1;"
        ).arg(trend.startsWith("↗") || trend.startsWith("+") ? "#22c55e" :
              trend.startsWith("↘") || trend.startsWith("-") ? "#ef4444" : "#6b7280"));

        trendLayout->addWidget(trendIcon);
        trendLayout->addWidget(trendValueLabel);
        trendLayout->addStretch();
        cardLayout->addLayout(trendLayout);
    }

    return card;
}

// ============== 图表区域 ==============
QWidget* StatisticsDialog::createChartsSection()
{
    tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            background-color: white;
            border-radius: 8px;
            border: 1px solid #e5e7eb;
            margin-top: 0px;
        }
    )");

    // 饼图 - 缺陷分布
    QWidget *pieTab = new QWidget();
    QVBoxLayout *pieLayout = new QVBoxLayout(pieTab);
    pieLayout->setContentsMargins(16, 16, 16, 16);
    pieLayout->addWidget(createPieChartView());
    tabWidget->addTab(pieTab, "缺陷分布");

    // 柱状图 - 数量对比
    QWidget *barTab = new QWidget();
    QVBoxLayout *barLayout = new QVBoxLayout(barTab);
    barLayout->setContentsMargins(16, 16, 16, 16);
    barLayout->addWidget(createBarChartView());
    tabWidget->addTab(barTab, "数量对比");

    // 置信度分布
    QWidget *confTab = new QWidget();
    QVBoxLayout *confLayout = new QVBoxLayout(confTab);
    confLayout->setContentsMargins(16, 16, 16, 16);
    confLayout->addWidget(createConfidenceChartView());
    tabWidget->addTab(confTab, "置信度分布");

    // 雷达图 - 综合评估
    QWidget *radarTab = new QWidget();
    QVBoxLayout *radarLayout = new QVBoxLayout(radarTab);
    radarLayout->setContentsMargins(16, 16, 16, 16);
    radarLayout->addWidget(createRadarChartView());
    tabWidget->addTab(radarTab, "综合评估");

    // 趋势图
    QWidget *trendTab = new QWidget();
    QVBoxLayout *trendLayout = new QVBoxLayout(trendTab);
    trendLayout->setContentsMargins(16, 16, 16, 16);
    trendLayout->addWidget(createTrendChartView());
    tabWidget->addTab(trendTab, "趋势分析");

    connect(tabWidget, &QTabWidget::currentChanged, this, &StatisticsDialog::onTabChanged);

    return tabWidget;
}

QChartView* StatisticsDialog::createPieChartView()
{
    if (statistics.defectCounts.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无缺陷数据");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #9ca3af; font-size: 16px; padding: 40px;");
        QChartView *view = new QChartView();
        view->setStyleSheet("background-color: white; border-radius: 8px;");
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(emptyLabel);
        view->setLayout(layout);
        return view;
    }

    QPieSeries *series = new QPieSeries();
    series->setHoleSize(0.45);  // 环形图效果

    int colorIndex = 0;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        QString label = QString("%1: %2").arg(it.key()).arg(it.value());
        QPieSlice *slice = series->append(label, it.value());

        QColor color = ChartColors::DEFECT_COLORS.value(it.key(),
                          ChartColors::PALETTE[colorIndex % ChartColors::PALETTE.size()]);
        slice->setColor(color);
        slice->setLabelVisible(false);

        // 悬停效果
        connect(slice, &QPieSlice::hovered, this, [slice](bool state) {
            slice->setLabelVisible(state);
            slice->setExploded(state);
        });

        colorIndex++;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Arial", 10));

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: white; border-radius: 8px;");

    return chartView;
}

QChartView* StatisticsDialog::createBarChartView()
{
    if (statistics.defectCounts.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无缺陷数据");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #9ca3af; font-size: 16px; padding: 40px;");
        QChartView *view = new QChartView();
        view->setStyleSheet("background-color: white; border-radius: 8px;");
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(emptyLabel);
        view->setLayout(layout);
        return view;
    }

    QBarSeries *series = new QBarSeries();

    QStringList categories;
    QList<qreal> values;

    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        categories.append(it.key());
        values.append(it.value());
    }

    QBarSet *barSet = new QBarSet("缺陷数量");
    for (qreal value : values) {
        barSet->append(value);
    }

    // 设置渐变色
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setColorAt(0, colors.primary);
    gradient.setColorAt(1, colors.primary.darker(120));
    barSet->setColor(QColor::fromRgb(99, 102, 241));

    series->append(barSet);
    series->setLabelsVisible(true);
    series->setLabelsPosition(QAbstractBarSeries::LabelsCenter);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(false);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor(75, 85, 99));
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsColor(QColor(75, 85, 99));
    axisY->setGridLineColor(QColor(243, 244, 246));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: white; border-radius: 8px;");

    return chartView;
}

QChartView* StatisticsDialog::createConfidenceChartView()
{
    QVector<float> allConfidences;
    for (auto it = statistics.defectConfidences.begin(); it != statistics.defectConfidences.end(); ++it) {
        allConfidences.append(it.value());
    }

    QMap<QString, QPair<int, int>> distribution = calculateConfidenceDistribution(allConfidences);

    QBarSeries *series = new QBarSeries();

    QStringList categories;
    QList<qreal> values;

    for (auto it = distribution.begin(); it != distribution.end(); ++it) {
        categories.append(it.key());
        values.append(it.value().first);
    }

    QBarSet *barSet = new QBarSet("样本数量");
    for (qreal value : values) {
        barSet->append(value);
    }
    barSet->setColor(colors.cyan);

    series->append(barSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(false);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor(75, 85, 99));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsColor(QColor(75, 85, 99));
    axisY->setGridLineColor(QColor(243, 244, 246));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: white; border-radius: 8px;");

    return chartView;
}

QChartView* StatisticsDialog::createRadarChartView()
{
    // 计算各维度数据
    int totalDefects = 0;
    QVector<float> allConfidences;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        totalDefects += it.value();
        allConfidences.append(statistics.defectConfidences.value(it.key()));
    }

    double defectRate = statistics.totalImages > 0 ?
                        (double)statistics.imagesWithDefects / statistics.totalImages * 100 : 0;
    double passRate = 100.0 - defectRate;
    double avgConfidence = calculateAverageConfidence(allConfidences);
    double confidenceStd = calculateStdDeviation(allConfidences);
    double qualityScore = qualityAssessment.overallScore;

    // 雷达图数据（归一化到0-100）
    QVector<QPair<QString, qreal>> radarData = {
        {"合格率", passRate},
        {"平均置信度", avgConfidence * 100},
        {"质量评分", qualityScore},
        {"一致性", (1.0 - confidenceStd) * 100},
        {"检出率", (double)statistics.imagesWithDefects / (statistics.totalImages + 0.1) * 100},
    };

    QLineSeries *series = new QLineSeries();
    series->setName("质量评估");

    // 使用多边形方式绘制（替代极坐标）
    int count = radarData.size();
    for (int i = 0; i < count; i++) {
        qreal angle = (2 * M_PI * i) / count - M_PI / 2;  // 从顶部开始
        qreal value = radarData[i].second / 100.0;  // 归一化到0-1
        qreal x = 100 + 100 * value * qCos(angle);
        qreal y = 100 + 100 * value * qSin(angle);
        series->append(x, y);
    }
    // 闭合多边形
    series->append(series->at(0));

    // 设置样式
    QPen pen(colors.primary);
    pen.setWidth(3);
    series->setPen(pen);
    series->setPointsVisible(true);

    // 填充区域
    QAreaSeries *areaSeries = new QAreaSeries(series);
    QLinearGradient gradient(0.5, 0, 0.5, 1);
    gradient.setColorAt(0, QColor(99, 102, 241, 150));
    gradient.setColorAt(1, QColor(99, 102, 241, 50));
    areaSeries->setBrush(QBrush(gradient));

    QChart *chart = new QChart();
    chart->addSeries(areaSeries);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(false);

    // 设置坐标轴
    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(0, 200);
    axisX->setLabelsVisible(false);
    axisX->setGridLineColor(QColor(243, 244, 246));
    chart->addAxis(axisX, Qt::AlignBottom);
    areaSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 200);
    axisY->setLabelsVisible(false);
    axisY->setGridLineColor(QColor(243, 244, 246));
    chart->addAxis(axisY, Qt::AlignLeft);
    areaSeries->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: white; border-radius: 8px;");

    return chartView;
}

QChartView* StatisticsDialog::createTrendChartView()
{
    QLineSeries *series = new QLineSeries();
    series->setName("检测趋势");

    // 模拟趋势数据（实际应用中应该从历史数据加载）
    int maxValue = 0;
    for (int i = 0; i < 10; i++) {
        int value = rand() % 50 + 10;
        maxValue = qMax(maxValue, value);
        series->append(i, value);
    }

    // 设置样式
    QPen pen(colors.primary);
    pen.setWidth(3);
    series->setPen(pen);
    series->setPointsVisible(true);
    series->setPointLabelsVisible(false);

    // 渐变填充
    QAreaSeries *areaSeries = new QAreaSeries(series);
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setColorAt(0, QColor(99, 102, 241, 100));
    gradient.setColorAt(1, QColor(99, 102, 241, 10));
    areaSeries->setBrush(QBrush(gradient));

    QChart *chart = new QChart();
    chart->addSeries(areaSeries);
    chart->setBackgroundVisible(false);
    chart->legend()->setVisible(false);

    QValueAxis *axisX = new QValueAxis();
    axisX->setRange(0, 9);
    axisX->setLabelsColor(QColor(75, 85, 99));
    axisX->setGridLineColor(QColor(243, 244, 246));
    chart->addAxis(axisX, Qt::AlignBottom);
    areaSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxValue * 1.2);
    axisY->setLabelsColor(QColor(75, 85, 99));
    axisY->setGridLineColor(QColor(243, 244, 246));
    chart->addAxis(axisY, Qt::AlignLeft);
    areaSeries->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: white; border-radius: 8px;");

    return chartView;
}

// ============== 质量评估 ==============
QWidget* StatisticsDialog::createQualityAssessment()
{
    QFrame *card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border-radius: 12px;
            border: 1px solid #e5e7eb;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("质量综合评估");
    QFont titleFont;
    titleFont.setPointSize(16);
    titleFont.setWeight(QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #111827;");
    layout->addWidget(titleLabel);

    // 评估内容
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(30);

    // 左侧：评分仪表盘
    QWidget *gaugeWidget = new QWidget();
    QVBoxLayout *gaugeLayout = new QVBoxLayout(gaugeWidget);

    // 评分显示
    QLabel *scoreLabel = new QLabel(QString::number(qualityAssessment.overallScore, 'f', 0));
    scoreLabel->setStyleSheet(QString(R"(
        QLabel {
            font-size: 64px;
            font-weight: bold;
            color: %1;
            background-color: rgba(%2, %3, %4, 10);
            padding: 30px;
            border-radius: 50%%;
        }
    )").arg(qualityAssessment.overallScore >= 90 ? "#22c55e" :
            qualityAssessment.overallScore >= 70 ? "#f59e0b" :
            qualityAssessment.overallScore >= 50 ? "#6366f1" : "#ef4444")
         .arg(colors.primary.red()).arg(colors.primary.green()).arg(colors.primary.blue()));
    scoreLabel->setAlignment(Qt::AlignCenter);

    QLabel *gradeLabel = new QLabel(QString("等级: %1").arg(qualityAssessment.grade));
    gradeLabel->setStyleSheet("font-size: 18px; color: #6b7280; margin-top: 10px;");
    gradeLabel->setAlignment(Qt::AlignCenter);

    gaugeLayout->addWidget(scoreLabel, 0, Qt::AlignCenter);
    gaugeLayout->addWidget(gradeLabel, 0, Qt::AlignCenter);
    gaugeLayout->setAlignment(scoreLabel, Qt::AlignCenter);
    contentLayout->addWidget(gaugeWidget, 1);

    // 右侧：详细指标
    QWidget *metricsWidget = new QWidget();
    QVBoxLayout *metricsLayout = new QVBoxLayout(metricsWidget);
    metricsLayout->setSpacing(16);

    struct Metric {
        QString name;
        double value;
        QString unit;
        QColor color;
    };

    QList<Metric> metrics = {
        {"合格率", 100.0 - qualityAssessment.detectionRate, "%", colors.success},
        {"平均置信度", qualityAssessment.avgConfidence * 100, "%", colors.primary},
        {"检出图片数", static_cast<double>(statistics.imagesWithDefects), "张", colors.warning},
        {"缺陷总数", 0, "", colors.danger},  // 计算总缺陷数
    };

    int totalDefects = 0;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        totalDefects += it.value();
    }
    metrics[3].value = totalDefects;

    for (const Metric &metric : metrics) {
        QWidget *metricRow = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout(metricRow);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(12);

        QLabel *nameLabel = new QLabel(metric.name);
        nameLabel->setStyleSheet("color: #374151; font-size: 14px; min-width: 100px;");

        QProgressBar *progress = new QProgressBar();
        progress->setValue(metric.value > 100 ? 100 : (int)metric.value);
        progress->setStyleSheet(QString(R"(
            QProgressBar {
                background-color: #f3f4f6;
                border-radius: 4px;
                height: 8px;
                text-align: center;
            }
            QProgressBar::chunk {
                background-color: %1;
                border-radius: 4px;
            }
        )").arg(metric.color.name()));
        progress->setFixedWidth(150);

        QLabel *valueLabel = new QLabel(QString("%1%2").arg(metric.value, 0, 'f', 1).arg(metric.unit));
        valueLabel->setStyleSheet("color: #111827; font-weight: 600; min-width: 60px;");

        rowLayout->addWidget(nameLabel);
        rowLayout->addWidget(progress);
        rowLayout->addWidget(valueLabel);
        rowLayout->addStretch();

        metricsLayout->addWidget(metricRow);
    }

    // 改进建议
    if (!qualityAssessment.recommendation.isEmpty()) {
        QLabel *recommendLabel = new QLabel(QString("💡 改进建议: %1").arg(qualityAssessment.recommendation));
        recommendLabel->setStyleSheet(QString(R"(
            QLabel {
                background-color: rgba(%1, %2, %3, 10);
                color: %1;
                padding: 12px 16px;
                border-radius: 8px;
                font-size: 13px;
                margin-top: 10px;
            }
        )").arg(colors.primary.red()).arg(colors.primary.green()).arg(colors.primary.blue()));
        metricsLayout->addWidget(recommendLabel);
    }

    contentLayout->addWidget(metricsWidget, 2);
    layout->addLayout(contentLayout);

    return card;
}

// ============== 详细数据表格 ==============
QWidget* StatisticsDialog::createDetailsSection()
{
    QFrame *card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border-radius: 12px;
            border: 1px solid #e5e7eb;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(16);

    // 标题
    QLabel *titleLabel = new QLabel("缺陷详细统计");
    QFont titleFont;
    titleFont.setPointSize(16);
    titleFont.setWeight(QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #111827;");
    layout->addWidget(titleLabel);

    // 表格
    detailsTable = new QTableWidget();
    detailsTable->setColumnCount(6);
    detailsTable->setHorizontalHeaderLabels({
        "缺陷类型", "数量", "占比", "影响图片", "平均置信度", "标准差"
    });

    int totalDefects = 0;
    for (int count : statistics.defectCounts) {
        totalDefects += count;
    }

    int row = 0;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        QString defectType = it.key();
        int count = it.value();
        double ratio = totalDefects > 0 ? (double)count / totalDefects * 100 : 0;
        int imageCount = statistics.defectImageCounts.value(defectType, 0);
        QVector<float> confidences = statistics.defectConfidences.value(defectType);
        double avgConf = calculateAverageConfidence(confidences);
        double stdDev = calculateStdDeviation(confidences);

        detailsTable->insertRow(row);

        // 缺陷类型（带颜色）
        QWidget *typeWidget = new QWidget();
        QHBoxLayout *typeLayout = new QHBoxLayout(typeWidget);
        typeLayout->setContentsMargins(8, 4, 8, 4);

        QColor color = ChartColors::DEFECT_COLORS.value(defectType, colors.primary);
        QLabel *colorDot = new QLabel();
        colorDot->setFixedSize(10, 10);
        colorDot->setStyleSheet(QString(
            "background-color: %1; border-radius: 5px;"
        ).arg(color.name()));

        QLabel *typeLabel = new QLabel(defectType);
        typeLabel->setStyleSheet("color: #374151; font-weight: 500;");

        typeLayout->addWidget(colorDot);
        typeLayout->addWidget(typeLabel);
        typeLayout->addStretch();

        detailsTable->setCellWidget(row, 0, typeWidget);

        // 其他列
        detailsTable->setItem(row, 1, new QTableWidgetItem(QString::number(count)));
        detailsTable->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(ratio, 0, 'f', 1)));
        detailsTable->setItem(row, 3, new QTableWidgetItem(QString::number(imageCount)));
        detailsTable->setItem(row, 4, new QTableWidgetItem(QString::number(avgConf, 'f', 3)));
        detailsTable->setItem(row, 5, new QTableWidgetItem(QString::number(stdDev, 'f', 3)));

        // 样式
        for (int col = 1; col < 6; col++) {
            QTableWidgetItem *item = detailsTable->item(row, col);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
                item->setForeground(QColor(75, 85, 99));
            }
        }

        row++;
    }

    detailsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    detailsTable->setColumnWidth(0, 120);
    detailsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    detailsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    detailsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    detailsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    detailsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    connect(detailsTable->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &StatisticsDialog::onSortColumn);

    layout->addWidget(detailsTable);

    return card;
}

// ============== 底部摘要 ==============
QWidget* StatisticsDialog::createSummarySection()
{
    QFrame *card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(99, 102, 241, 0.1),
                stop:1 rgba(168, 85, 247, 0.1));
            border-radius: 12px;
            border: 1px solid rgba(99, 102, 241, 0.2);
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);

    // 左侧：统计摘要
    QVBoxLayout *summaryLayout = new QVBoxLayout();

    int totalDefects = 0;
    QVector<float> allConfidences;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        totalDefects += it.value();
        allConfidences.append(statistics.defectConfidences.value(it.key()));
    }

    double avgConfidence = calculateAverageConfidence(allConfidences);

    QString summaryText = QString(
        "<span style='color: #6b7280; font-size: 13px;'>本次检测共处理 <b style='color: #111827;'>%1</b> 张图片，"
        "发现 <b style='color: #ef4444;'>%2</b> 张存在缺陷，"
        "共检测到 <b style='color: #111827;'>%3</b> 处缺陷。</span><br>"
        "<span style='color: #6b7280; font-size: 13px;'>检测置信度平均为 <b style='color: #6366f1;'>%4</b>，"
        "质量等级评定为 <b style='color: %5;'>%6</b>。</span>"
    ).arg(statistics.totalImages)
     .arg(statistics.imagesWithDefects)
     .arg(totalDefects)
     .arg(QString::number(avgConfidence, 'f', 3))
     .arg(qualityAssessment.overallScore >= 90 ? "#22c55e" :
          qualityAssessment.overallScore >= 70 ? "#f59e0b" : "#ef4444")
     .arg(qualityAssessment.grade);

    QLabel *summaryLabel = new QLabel(summaryText);
    summaryLabel->setTextFormat(Qt::RichText);
    summaryLabel->setStyleSheet("font-size: 14px;");
    summaryLayout->addWidget(summaryLabel);

    layout->addLayout(summaryLayout);

    // 右侧：时间戳
    QLabel *timestampLabel = new QLabel(
        QString("报告生成: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
    );
    timestampLabel->setStyleSheet("color: #9ca3af; font-size: 12px;");
    layout->addWidget(timestampLabel);

    return card;
}

// ============== 辅助方法 ==============
StatisticsDialog::QualityAssessment StatisticsDialog::calculateQualityAssessment()
{
    QualityAssessment assessment;

    int totalDefects = 0;
    QVector<float> allConfidences;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        totalDefects += it.value();
        allConfidences.append(statistics.defectConfidences.value(it.key()));
    }

    // 计算检出率
    assessment.detectionRate = statistics.totalImages > 0 ?
                               (double)statistics.imagesWithDefects / statistics.totalImages * 100 : 0;

    // 计算平均置信度
    assessment.avgConfidence = calculateAverageConfidence(allConfidences);

    // 计算综合评分 (0-100)
    double confidenceScore = assessment.avgConfidence * 100;  // 置信度权重 40%
    double passScore = (100.0 - assessment.detectionRate) * 0.3;  // 合格率权重 30%
    double stabilityScore = (1.0 - calculateStdDeviation(allConfidences)) * 30;  // 稳定性权重 30%

    assessment.overallScore = confidenceScore * 0.4 + passScore * 0.3 + stabilityScore * 0.3;
    assessment.overallScore = qBound(0.0, assessment.overallScore, 100.0);

    // 等级评定
    if (assessment.overallScore >= 90) {
        assessment.grade = "A 优秀";
    } else if (assessment.overallScore >= 75) {
        assessment.grade = "B 良好";
    } else if (assessment.overallScore >= 60) {
        assessment.grade = "C 合格";
    } else {
        assessment.grade = "D 待改进";
    }

    // 改进建议
    if (assessment.avgConfidence < 0.7) {
        assessment.recommendation = "建议优化模型参数或检查图像质量，以提高检测置信度。";
    } else if (assessment.detectionRate > 50) {
        assessment.recommendation = "缺陷检出率较高，建议检查生产工艺或适当调整检测阈值。";
    } else if (calculateStdDeviation(allConfidences) > 0.15) {
        assessment.recommendation = "检测结果一致性较差，建议进行模型校准或环境优化。";
    } else {
        assessment.recommendation = "整体检测效果良好，继续保持！";
    }

    return assessment;
}

double StatisticsDialog::calculateAverageConfidence(const QVector<float> &confidences) const
{
    if (confidences.isEmpty()) return 0.0;
    double sum = 0;
    for (float c : confidences) sum += c;
    return sum / confidences.size();
}

double StatisticsDialog::calculateStdDeviation(const QVector<float> &confidences) const
{
    if (confidences.size() <= 1) return 0.0;
    double avg = calculateAverageConfidence(confidences);
    double sumSq = 0;
    for (float c : confidences) sumSq += (c - avg) * (c - avg);
    return std::sqrt(sumSq / confidences.size());
}

QMap<QString, QPair<int, int>> StatisticsDialog::calculateConfidenceDistribution(
    const QVector<float> &allConfidences) const
{
    QMap<QString, QPair<int, int>> distribution;
    distribution["0.0-0.5"] = qMakePair(0, 0);
    distribution["0.5-0.6"] = qMakePair(0, 0);
    distribution["0.6-0.7"] = qMakePair(0, 0);
    distribution["0.7-0.8"] = qMakePair(0, 0);
    distribution["0.8-0.9"] = qMakePair(0, 0);
    distribution["0.9-1.0"] = qMakePair(0, 0);

    for (float confidence : allConfidences) {
        if (confidence < 0.5) {
            distribution["0.0-0.5"].first++;
        } else if (confidence < 0.6) {
            distribution["0.5-0.6"].first++;
        } else if (confidence < 0.7) {
            distribution["0.6-0.7"].first++;
        } else if (confidence < 0.8) {
            distribution["0.7-0.8"].first++;
        } else if (confidence < 0.9) {
            distribution["0.8-0.9"].first++;
        } else {
            distribution["0.9-1.0"].first++;
        }
    }

    return distribution;
}

// ============== 槽函数 ==============
void StatisticsDialog::onExportPDF()
{
    // PDF导出（简化实现）
    QMessageBox::information(this, "导出", "PDF导出功能将在后续版本中实现。\n当前可使用截图功能保存报告。");
}

void StatisticsDialog::onExportCSV()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出CSV",
        QString("defect_statistics_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
        "CSV Files (*.csv)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        // 写入CSV头部
        stream << "缺陷类型,数量,占比,影响图片数,平均置信度\n";

        int totalDefects = 0;
        for (int count : statistics.defectCounts) {
            totalDefects += count;
        }

        for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
            QString defectType = it.key();
            int count = it.value();
            double ratio = totalDefects > 0 ? (double)count / totalDefects * 100 : 0;
            int imageCount = statistics.defectImageCounts.value(defectType, 0);
            double avgConf = calculateAverageConfidence(statistics.defectConfidences.value(defectType));

            stream << QString("%1,%2,%3%,%4,%5\n")
                      .arg(defectType)
                      .arg(count)
                      .arg(ratio, 0, 'f', 1)
                      .arg(imageCount)
                      .arg(avgConf, 0, 'f', 3);
        }

        file.close();
        QMessageBox::information(this, "导出成功", QString("数据已导出到: %1").arg(filePath));
    }
}

void StatisticsDialog::onRefresh()
{
    // 刷新图表数据
    qualityAssessment = calculateQualityAssessment();
    QMessageBox::information(this, "刷新", "数据已刷新");
}

void StatisticsDialog::onTabChanged(int index)
{
    // 切换选项卡时的处理
    qDebug() << "切换到选项卡:" << index;
}

void StatisticsDialog::onSortColumn(int column)
{
    if (sortColumn == column) {
        sortAscending = !sortAscending;
    } else {
        sortColumn = column;
        sortAscending = true;
    }

    detailsTable->sortItems(column, sortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void StatisticsDialog::onSearchFilter(const QString &text)
{
    for (int row = 0; row < detailsTable->rowCount(); ++row) {
        bool match = text.isEmpty();
        for (int col = 0; col < detailsTable->columnCount(); ++col) {
            QWidget *widget = detailsTable->cellWidget(row, col);
            if (widget) {
                QLabel *label = qobject_cast<QLabel*>(widget);
                if (label && label->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            } else {
                QTableWidgetItem *item = detailsTable->item(row, col);
                if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }
        detailsTable->setRowHidden(row, !match);
    }
}
