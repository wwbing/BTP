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
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsLayout>

QT_CHARTS_USE_NAMESPACE

// 现代配色方案
namespace Colors {
    const QColor PRIMARY = QColor(79, 70, 229);       // 靛蓝
    const QColor SUCCESS = QColor(16, 185, 129);      // 翠绿
    const QColor WARNING = QColor(245, 158, 11);      // 琥珀
    const QColor DANGER = QColor(239, 68, 68);        // 珊瑚红
    const QColor INFO = QColor(59, 130, 246);         // 天蓝
    const QColor PURPLE = QColor(139, 92, 246);       // 紫色
    const QColor CYAN = QColor(6, 182, 212);          // 青色

    // 渐变色
    const QList<QColor> CHART_COLORS = {
        QColor(99, 102, 241),   // 靛蓝
        QColor(236, 72, 153),   // 粉红
        QColor(16, 185, 129),   // 翠绿
        QColor(245, 158, 11),   // 琥珀
        QColor(59, 130, 246),   // 天蓝
        QColor(139, 92, 246)    // 紫色
    };
}

StatisticsDialog::StatisticsDialog(const DefectStatistics &stats, QWidget *parent)
    : QDialog(parent), statistics(stats)
{
    setWindowTitle("缺陷检测统计分析");
    setMinimumSize(1400, 850);
    resize(1600, 950);

    setupUI();
}

StatisticsDialog::~StatisticsDialog()
{
}

void StatisticsDialog::setupUI()
{
    // 设置主窗口样式
    setStyleSheet(R"(
        QDialog {
            background-color: #f8fafc;
        }
        QPushButton {
            background-color: #4f46e5;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 24px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: #4338ca;
        }
        QPushButton:pressed {
            background-color: #3730a3;
        }
    )");

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 标题栏
    QWidget *titleBar = new QWidget();
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *titleLabel = new QLabel("缺陷检测统计分析报告");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #1e293b;");

    QLabel *subtitleLabel = new QLabel("智能质量检测系统 | 批量检测结果");
    subtitleLabel->setStyleSheet("color: #64748b; font-size: 13px;");

    QVBoxLayout *titleTextLayout = new QVBoxLayout();
    titleTextLayout->addWidget(titleLabel);
    titleTextLayout->addWidget(subtitleLabel);
    titleTextLayout->setSpacing(4);

    titleLayout->addLayout(titleTextLayout);
    titleLayout->addStretch();

    mainLayout->addWidget(titleBar);

    // 创建滚动区域和仪表板
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

    QWidget *dashboard = createDashboard();
    scrollArea->setWidget(dashboard);

    mainLayout->addWidget(scrollArea);

    // 底部按钮栏
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 8, 0, 0);

    QPushButton *exportButton = new QPushButton("导出报告");
    exportButton->setStyleSheet(R"(
        QPushButton {
            background-color: #10b981;
        }
        QPushButton:hover {
            background-color: #059669;
        }
    )");

    QPushButton *closeButton = new QPushButton("关闭");
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #64748b;
        }
        QPushButton:hover {
            background-color: #475569;
        }
    )");
    connect(closeButton, &QPushButton::clicked, this, &StatisticsDialog::close);

    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
    buttonLayout->addSpacing(12);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

QWidget* StatisticsDialog::createDashboard()
{
    QWidget *dashboard = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(dashboard);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // KPI指标卡片
    layout->addWidget(createKpiCards());

    // 图表行
    layout->addWidget(createChartsRow());

    // 详情行
    layout->addWidget(createDetailsRow());

    layout->addStretch();

    return dashboard;
}

QWidget* StatisticsDialog::createKpiCards()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    int totalDefects = 0;
    for (int count : statistics.defectCounts) {
        totalDefects += count;
    }

    double defectRate = statistics.totalImages > 0 ?
                      (double)statistics.imagesWithDefects / statistics.totalImages * 100 : 0;

    double avgDefectsPerImage = statistics.imagesWithDefects > 0 ?
                               (double)totalDefects / statistics.imagesWithDefects : 0;

    QVector<float> allConfidences;
    for (auto it = statistics.defectConfidences.begin(); it != statistics.defectConfidences.end(); ++it) {
        allConfidences.append(it.value());
    }
    double avgConfidence = calculateAverageConfidence(allConfidences);

    // 创建KPI卡片
    layout->addWidget(createKpiCard("总检测图片", QString::number(statistics.totalImages), "张",
                                   "📷", Colors::INFO), 1);
    layout->addWidget(createKpiCard("缺陷图片数", QString::number(statistics.imagesWithDefects), "张",
                                   "🔍", Colors::WARNING), 1);
    layout->addWidget(createKpiCard("缺陷检出率", QString("%1%").arg(defectRate, 0, 'f', 1), "",
                                   "📊", Colors::PRIMARY), 1);
    layout->addWidget(createKpiCard("缺陷总数", QString::number(totalDefects), "个",
                                   "⚠️", Colors::DANGER), 1);
    layout->addWidget(createKpiCard("平均置信度", QString("%1").arg(avgConfidence, 0, 'f', 3), "",
                                   "✓", Colors::SUCCESS), 1);

    return container;
}

QWidget* StatisticsDialog::createKpiCard(const QString &title, const QString &value,
                                         const QString &unit, const QString &icon,
                                         const QColor &accentColor)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(getCardStyle(accentColor));

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 16, 20, 16);
    cardLayout->setSpacing(8);

    // 标题和图标
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QLabel *iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet(QString("font-size: 28px; background-color: %1; padding: 8px; "
                                     "border-radius: 12px; min-width: 48px; min-height: 48px;")
                             .arg(QString("rgba(%1, %2, %3, 30)")
                                  .arg(accentColor.red())
                                  .arg(accentColor.green())
                                  .arg(accentColor.blue())));

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 500;");

    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel, 1);
    cardLayout->addLayout(headerLayout);

    // 数值
    QHBoxLayout *valueLayout = new QHBoxLayout();
    valueLayout->setSpacing(4);

    QLabel *valueLabel = new QLabel(value);
    valueLabel->setStyleSheet(QString("color: %1; font-size: 32px; font-weight: 700; letter-spacing: -1px;")
                             .arg(QString("rgb(%1, %2, %3)")
                                  .arg(accentColor.red())
                                  .arg(accentColor.green())
                                  .arg(accentColor.blue())));

    QLabel *unitLabel = new QLabel(unit);
    unitLabel->setStyleSheet("color: #94a3b8; font-size: 14px; font-weight: 500;");
    unitLabel->setAlignment(Qt::AlignBottom);

    valueLayout->addWidget(valueLabel);
    valueLayout->addWidget(unitLabel);
    valueLayout->addStretch();
    cardLayout->addLayout(valueLayout);

    return card;
}

QWidget* StatisticsDialog::createChartsRow()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    // 饼图 - 缺陷分布
    QWidget *pieCard = createChartCard("缺陷类型分布", createDefectPieChart());
    layout->addWidget(pieCard, 1);

    // 柱状图 - 缺陷对比
    QWidget *barCard = createChartCard("各类型缺陷数量对比", createDefectBarChart());
    layout->addWidget(barCard, 1);

    return container;
}

QWidget* StatisticsDialog::createChartCard(const QString &title, QWidget *chartWidget)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border-radius: 16px;
            border: 1px solid #e2e8f0;
        }
    )");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 16, 20, 16);
    cardLayout->setSpacing(12);

    // 标题
    QLabel *titleLabel = new QLabel(title);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #1e293b; padding-left: 8px; border-left: 4px solid #4f46e5;");

    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(chartWidget, 1);

    return card;
}

QWidget* StatisticsDialog::createDetailsRow()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    // 置信度分布图
    QWidget *confidenceCard = createChartCard("置信度分布分析", createConfidenceChart());
    layout->addWidget(confidenceCard, 1);

    // 详细统计表格
    QWidget *tableCard = createChartCard("缺陷详细统计", createDefectDetailsTable());
    layout->addWidget(tableCard, 1);

    return container;
}

QWidget* StatisticsDialog::createDefectPieChart()
{
    if (statistics.defectCounts.isEmpty()) {
        return createEmptyWidget("暂无缺陷数据");
    }

    QPieSeries *series = new QPieSeries();

    int colorIndex = 0;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        QString label = QString("%1\n%2").arg(it.key()).arg(it.value());
        QPieSlice *slice = series->append(label, it.value());

        if (colorIndex < Colors::CHART_COLORS.size()) {
            slice->setColor(Colors::CHART_COLORS[colorIndex]);
        }
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelPosition::LabelOutside);
        slice->setLabelBrush(QBrush(QColor(51, 65, 85)));

        QFont labelFont = slice->labelFont();
        labelFont.setPointSize(11);
        labelFont.setBold(true);
        slice->setLabelFont(labelFont);

        colorIndex++;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->setVisible(false);
    chart->setBackgroundRoundness(0);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background-color: transparent; border: none;");

    return chartView;
}

QWidget* StatisticsDialog::createDefectBarChart()
{
    if (statistics.defectCounts.isEmpty()) {
        return createEmptyWidget("暂无缺陷数据");
    }

    QBarSeries *series = new QBarSeries();
    QBarSet *defectSet = new QBarSet("缺陷数量");

    // 设置柱状图颜色
    defectSet->setColor(Colors::PRIMARY);
    defectSet->setBorderColor(Colors::PRIMARY);
    defectSet->setLabelColor(QColor(51, 65, 85));

    QStringList categories;
    QList<int> values;

    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        categories.append(it.key());
        values.append(it.value());
    }

    QList<qreal> realValues;
    for (int value : values) {
        realValues.append(value);
    }
    defectSet->append(realValues);
    series->append(defectSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setMargins(QMargins(10, 10, 10, 20));
    chart->legend()->setVisible(false);
    chart->setBackgroundRoundness(0);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    axis->setGridLineVisible(false);
    axis->setLabelsColor(QColor(71, 85, 105));
    axis->setLabelsFont(QFont("Arial", 10));
    chart->addAxis(axis, Qt::AlignBottom);
    series->attachAxis(axis);

    QValueAxis *valueAxis = new QValueAxis();
    valueAxis->setGridLineVisible(true);
    valueAxis->setGridLineColor(QColor(241, 245, 249));
    valueAxis->setLabelsColor(QColor(71, 85, 105));
    chart->addAxis(valueAxis, Qt::AlignLeft);
    series->attachAxis(valueAxis);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    chartView->setStyleSheet("background-color: transparent; border: none;");

    return chartView;
}

QWidget* StatisticsDialog::createConfidenceChart()
{
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    if (statistics.defectConfidences.isEmpty()) {
        layout->addWidget(createEmptyWidget("暂无置信度数据"));
        return container;
    }

    QVector<float> allConfidences;
    for (auto it = statistics.defectConfidences.begin(); it != statistics.defectConfidences.end(); ++it) {
        allConfidences.append(it.value());
    }

    if (allConfidences.isEmpty()) {
        layout->addWidget(createEmptyWidget("暂无置信度数据"));
        return container;
    }

    QMap<QString, QPair<int, int>> distribution = calculateTotalConfidenceDistribution(allConfidences);

    QBarSeries *series = new QBarSeries();
    QBarSet *countSet = new QBarSet("样本数量");
    countSet->setColor(Colors::CYAN);
    countSet->setBorderColor(Colors::CYAN);

    QStringList categories;
    QList<int> values;

    for (auto it = distribution.begin(); it != distribution.end(); ++it) {
        categories.append(it.key());
        values.append(it.value().first);
    }

    QList<qreal> realValues;
    for (int value : values) {
        realValues.append(value);
    }
    countSet->append(realValues);
    series->append(countSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setMargins(QMargins(10, 0, 10, 10));
    chart->legend()->setVisible(false);
    chart->setBackgroundRoundness(0);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    axis->setGridLineVisible(false);
    axis->setLabelsColor(QColor(71, 85, 105));
    chart->addAxis(axis, Qt::AlignBottom);
    series->attachAxis(axis);

    QValueAxis *valueAxis = new QValueAxis();
    valueAxis->setGridLineVisible(true);
    valueAxis->setGridLineColor(QColor(241, 245, 249));
    valueAxis->setLabelsColor(QColor(71, 85, 105));
    chart->addAxis(valueAxis, Qt::AlignLeft);
    series->attachAxis(valueAxis);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(220);
    chartView->setStyleSheet("background-color: transparent; border: none;");

    layout->addWidget(chartView);

    // 统计信息
    QLabel *statsLabel = new QLabel();
    QString statsText = QString(
        "总样本: <b style='color: #4f46e5;'>%1</b> | "
        "平均: <b style='color: #10b981;'>%2</b> | "
        "最高: <b style='color: #059669;'>%3</b> | "
        "最低: <b style='color: #dc2626;'>%4</b>"
    ).arg(allConfidences.size())
     .arg(calculateAverageConfidence(allConfidences), 0, 'f', 3)
     .arg(*std::max_element(allConfidences.begin(), allConfidences.end()), 0, 'f', 3)
     .arg(*std::min_element(allConfidences.begin(), allConfidences.end()), 0, 'f', 3);

    statsLabel->setText(statsText);
    statsLabel->setStyleSheet(
        "QLabel { "
        "padding: 12px 16px; "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 rgba(79, 70, 229, 0.08), stop:1 rgba(16, 185, 129, 0.08)); "
        "border-radius: 8px; "
        "color: #475569; "
        "font-size: 13px; "
        "}"
    );
    layout->addWidget(statsLabel);

    return container;
}

QWidget* StatisticsDialog::createDefectDetailsTable()
{
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QTableWidget *table = new QTableWidget();
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"缺陷类型", "数量", "占比", "影响图片数", "平均置信度"});
    table->setStyleSheet(R"(
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: #f1f5f9;
            font-size: 13px;
        }
        QTableWidget::item {
            padding: 8px;
            border: none;
            border-bottom: 1px solid #f1f5f9;
        }
        QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            padding: 12px 8px;
            border: none;
            border-bottom: 2px solid #e2e8f0;
            font-weight: 600;
            font-size: 13px;
        }
        QTableWidget::item:selected {
            background-color: rgba(79, 70, 229, 0.1);
        }
    )");

    table->horizontalHeader()->setStretchLastSection(false);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

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

        table->insertRow(row);

        // 缺陷类型 - 带颜色标签
        QWidget *typeWidget = new QWidget();
        QHBoxLayout *typeLayout = new QHBoxLayout(typeWidget);
        typeLayout->setContentsMargins(8, 4, 8, 4);

        QLabel *colorDot = new QLabel();
        colorDot->setFixedSize(12, 12);
        int colorIdx = row % Colors::CHART_COLORS.size();
        colorDot->setStyleSheet(QString(
            "background-color: rgb(%1, %2, %3); border-radius: 6px;"
        ).arg(Colors::CHART_COLORS[colorIdx].red())
         .arg(Colors::CHART_COLORS[colorIdx].green())
         .arg(Colors::CHART_COLORS[colorIdx].blue()));

        QLabel *typeLabel = new QLabel(defectType);
        typeLabel->setStyleSheet("color: #1e293b; font-weight: 600;");

        typeLayout->addWidget(colorDot);
        typeLayout->addWidget(typeLabel);
        typeLayout->addStretch();

        table->setCellWidget(row, 0, typeWidget);
        table->setItem(row, 1, new QTableWidgetItem(QString::number(count)));
        table->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(ratio, 0, 'f', 1)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(imageCount)));
        table->setItem(row, 4, new QTableWidgetItem(QString("%1").arg(avgConf, 0, 'f', 3)));

        // 设置样式
        for (int col = 1; col <= 4; ++col) {
            QTableWidgetItem *item = table->item(row, col);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
                item->setForeground(QColor(51, 65, 85));
            }
        }

        row++;
    }

    // 设置列宽：第一列固定宽度，其他列自适应
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    // 设置第一列的固定宽度（确保缺陷类型完整显示）
    table->setColumnWidth(0, 180);

    layout->addWidget(table);

    return container;
}

QWidget* StatisticsDialog::createEmptyWidget(const QString &message)
{
    QLabel *label = new QLabel(message);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(
        "color: #94a3b8; font-size: 14px; padding: 40px; "
        "background-color: #f8fafc; border-radius: 8px; border: 2px dashed #e2e8f0;"
    );
    return label;
}

QMap<QString, QPair<int, int>> StatisticsDialog::calculateTotalConfidenceDistribution(const QVector<float> &allConfidences) const {
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

double StatisticsDialog::calculateAverageConfidence(const QVector<float> &confidences) const {
    if (confidences.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (float confidence : confidences) {
        sum += confidence;
    }

    return sum / confidences.size();
}

QMap<QString, double> StatisticsDialog::calculateDefectRatios() const {
    QMap<QString, double> ratios;

    int totalDefects = 0;
    for (int count : statistics.defectCounts) {
        totalDefects += count;
    }

    if (totalDefects == 0) {
        return ratios;
    }

    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        ratios[it.key()] = (double)it.value() / totalDefects * 100;
    }

    return ratios;
}

QString StatisticsDialog::getCardStyle(const QColor &accentColor) const {
    return QString(R"(
        QFrame {
            background-color: white;
            border-radius: 16px;
            border: 1px solid #e2e8f0;
        }
        QFrame:hover {
            border: 1px solid rgba(%1, %2, %3, 50);
            box-shadow: 0 4px 20px rgba(%1, %2, %3, 15);
        }
    )").arg(accentColor.red())
         .arg(accentColor.green())
         .arg(accentColor.blue());
}

QString StatisticsDialog::getSectionTitleStyle() const {
    return R"(
        color: #1e293b;
        font-size: 16px;
        font-weight: 600;
        padding-left: 12px;
        border-left: 4px solid #4f46e5;
    )";
}
