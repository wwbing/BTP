#include "statisticsdialog.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QSpacerItem>
#include <QDebug>
#include <QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QLegend>

QT_CHARTS_USE_NAMESPACE

StatisticsDialog::StatisticsDialog(const DefectStatistics &stats, QWidget *parent)
    : QDialog(parent), statistics(stats)
{
    setWindowTitle("批量检测统计结果");
    setMinimumSize(1000, 700);

    
    setupUI();
}

StatisticsDialog::~StatisticsDialog()
{
}

void StatisticsDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);

    // 创建标题
    QLabel *titleLabel = new QLabel("批量检测统计分析报告", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px; color: #333;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 创建选项卡
    tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    // 添加各个选项卡
    createSummaryTab();
    tabWidget->addTab(createDefectCountChart(), "缺陷数量统计");
    tabWidget->addTab(createDefectRatioChart(), "缺陷比例分析");
    tabWidget->addTab(createConfidenceDistributionChart(), "置信度分布");

    // 创建关闭按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *closeButton = new QPushButton("关闭", this);
    closeButton->setStyleSheet("QPushButton { padding: 8px 20px; font-weight: bold; }");
    connect(closeButton, &QPushButton::clicked, this, &StatisticsDialog::close);

    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);
}

void StatisticsDialog::createSummaryTab()
{
    QWidget *summaryWidget = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(summaryWidget);

    // 计算统计数据
    QMap<QString, double> defectRatios = calculateDefectRatios();

    // 创建基本统计信息组
    QGroupBox *basicGroup = new QGroupBox("基本统计信息", this);
    QGridLayout *basicLayout = new QGridLayout(basicGroup);

    basicLayout->addWidget(new QLabel("总图片数："), 0, 0);
    basicLayout->addWidget(new QLabel(QString::number(statistics.totalImages)), 0, 1);

    basicLayout->addWidget(new QLabel("有缺陷图片数："), 1, 0);
    basicLayout->addWidget(new QLabel(QString::number(statistics.imagesWithDefects)), 1, 1);

    double defectRate = statistics.totalImages > 0 ?
                      (double)statistics.imagesWithDefects / statistics.totalImages * 100 : 0;
    basicLayout->addWidget(new QLabel("缺陷图片占比："), 2, 0);
    basicLayout->addWidget(new QLabel(QString("%1%").arg(defectRate, 0, 'f', 1)), 2, 1);

    int totalDefects = 0;
    for (int count : statistics.defectCounts) {
        totalDefects += count;
    }

    basicLayout->addWidget(new QLabel("缺陷总数："), 3, 0);
    basicLayout->addWidget(new QLabel(QString::number(totalDefects)), 3, 1);

    double avgDefectsPerImage = statistics.imagesWithDefects > 0 ?
                               (double)totalDefects / statistics.imagesWithDefects : 0;
    basicLayout->addWidget(new QLabel("平均每张图片缺陷数："), 4, 0);
    basicLayout->addWidget(new QLabel(QString("%1").arg(avgDefectsPerImage, 0, 'f', 2)), 4, 1);

    gridLayout->addWidget(basicGroup, 0, 0, 1, 2);

    // 创建各类型缺陷统计组
    QGroupBox *defectDetailGroup = new QGroupBox("各类型缺陷详细统计", this);
    QVBoxLayout *defectDetailLayout = new QVBoxLayout(defectDetailGroup);

    // 创建表格显示详细信息
    QTabWidget *defectTableWidget = new QTabWidget(this);

    // 缺陷数量表格
    QWidget *countWidget = new QWidget();
    QGridLayout *countLayout = new QGridLayout(countWidget);
    countLayout->addWidget(new QLabel("缺陷类型"), 0, 0);
    countLayout->addWidget(new QLabel("数量"), 0, 1);
    countLayout->addWidget(new QLabel("占比"), 0, 2);
    countLayout->addWidget(new QLabel("影响图片数"), 0, 3);

    int row = 1;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        QString defectType = it.key();
        int count = it.value();
        double ratio = totalDefects > 0 ? (double)count / totalDefects * 100 : 0;
        int imageCount = statistics.defectImageCounts.value(defectType, 0);

        countLayout->addWidget(new QLabel(defectType), row, 0);
        countLayout->addWidget(new QLabel(QString::number(count)), row, 1);
        countLayout->addWidget(new QLabel(QString("%1%").arg(ratio, 0, 'f', 1)), row, 2);
        countLayout->addWidget(new QLabel(QString::number(imageCount)), row, 3);
        row++;
    }

    defectTableWidget->addTab(countWidget, "缺陷数量");
    defectDetailLayout->addWidget(defectTableWidget);

    gridLayout->addWidget(defectDetailGroup, 1, 0, 1, 2);

    // 添加弹簧
    gridLayout->setRowStretch(2, 1);

    tabWidget->addTab(summaryWidget, "汇总信息");
}

QWidget* StatisticsDialog::createDefectCountChart()
{
    if (statistics.defectCounts.isEmpty()) {
        QLabel *noDataLabel = new QLabel("暂无缺陷数据");
        noDataLabel->setAlignment(Qt::AlignCenter);
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->addWidget(noDataLabel);
        return new QChartView(new QChart(), widget);
    }

    QPieSeries *series = new QPieSeries();

    // 使用更醒目的颜色
    QList<QColor> colors = {
        QColor(255, 99, 132),   // 红色
        QColor(54, 162, 235),   // 蓝色
        QColor(255, 205, 86),   // 黄色
        QColor(75, 192, 192),   // 青色
        QColor(153, 102, 255),  // 紫色
        QColor(255, 159, 64)    // 橙色
    };

    int colorIndex = 0;
    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        QString label = QString("%1 (%2)").arg(it.key()).arg(it.value());
        QPieSlice *slice = series->append(label, it.value());

        if (colorIndex < colors.size()) {
            slice->setColor(colors[colorIndex]);
        }
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelPosition::LabelOutside);
        slice->setLabelBrush(QBrush(Qt::black));

        colorIndex++;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("各类型缺陷数量分布");
    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    return chartView;
}

QWidget* StatisticsDialog::createDefectRatioChart()
{
    if (statistics.defectCounts.isEmpty()) {
        QLabel *noDataLabel = new QLabel("暂无缺陷数据");
        noDataLabel->setAlignment(Qt::AlignCenter);
        QWidget *widget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->addWidget(noDataLabel);
        return new QChartView(new QChart(), widget);
    }

    QBarSeries *series = new QBarSeries();
    QBarSet *defectSet = new QBarSet("缺陷数量");

    QStringList categories;
    QList<int> values;

    for (auto it = statistics.defectCounts.begin(); it != statistics.defectCounts.end(); ++it) {
        categories.append(it.key());
        values.append(it.value());
    }

    // 将QList<int>转换为QList<qreal>
        QList<qreal> realValues;
        for (int value : values) {
            realValues.append(value);
        }
        defectSet->append(realValues);
    series->append(defectSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("各类型缺陷数量对比");

    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    chart->addAxis(axis, Qt::AlignBottom);
    series->attachAxis(axis);

    QValueAxis *valueAxis = new QValueAxis();
    chart->addAxis(valueAxis, Qt::AlignLeft);
    series->attachAxis(valueAxis);

    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    return chartView;
}

QWidget* StatisticsDialog::createConfidenceDistributionChart()
{
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);

    if (statistics.defectConfidences.isEmpty()) {
        QLabel *noDataLabel = new QLabel("暂无置信度数据");
        noDataLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(noDataLabel);
        return container;
    }

    // 收集所有缺陷类型的置信度数据，计算总的分布
    QVector<float> allConfidences;
    for (auto it = statistics.defectConfidences.begin(); it != statistics.defectConfidences.end(); ++it) {
        allConfidences.append(it.value());
    }

    if (allConfidences.isEmpty()) {
        QLabel *noDataLabel = new QLabel("暂无置信度数据");
        noDataLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(noDataLabel);
        return container;
    }

    // 计算总的置信度分布
    QMap<QString, QPair<int, int>> totalDistribution = calculateTotalConfidenceDistribution(allConfidences);

    // 创建总的置信度分布图
    QWidget *chartWidget = createHistogram(totalDistribution, "总体置信度分布");
    layout->addWidget(chartWidget);

    // 添加统计信息
    QLabel *statsLabel = new QLabel();
    QString statsText = QString("总样本数: %1\n"
                               "平均置信度: %2\n"
                               "最高置信度: %3\n"
                               "最低置信度: %4")
                          .arg(allConfidences.size())
                          .arg(calculateAverageConfidence(allConfidences), 0, 'f', 3)
                          .arg(*std::max_element(allConfidences.begin(), allConfidences.end()), 0, 'f', 3)
                          .arg(*std::min_element(allConfidences.begin(), allConfidences.end()), 0, 'f', 3);

    statsLabel->setText(statsText);
    statsLabel->setStyleSheet("QLabel { padding: 10px; background-color: #f0f0f0; border-radius: 5px; }");
    layout->addWidget(statsLabel);

    layout->addStretch();

    return container;
}

QWidget* StatisticsDialog::createHistogram(const QMap<QString, QPair<int, int>> &distribution, const QString &defectType)
{
    QBarSeries *series = new QBarSeries();
    QBarSet *countSet = new QBarSet("数量");

    QStringList categories;
    QList<int> values;

    for (auto it = distribution.begin(); it != distribution.end(); ++it) {
        categories.append(it.key());
        values.append(it.value().first);
    }

    // 将QList<int>转换为QList<qreal>
    QList<qreal> realValues;
    for (int value : values) {
        realValues.append(value);
    }
    countSet->append(realValues);
    series->append(countSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("%1 置信度分布").arg(defectType));

    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    chart->addAxis(axis, Qt::AlignBottom);
    series->attachAxis(axis);

    QValueAxis *valueAxis = new QValueAxis();
    chart->addAxis(valueAxis, Qt::AlignLeft);
    series->attachAxis(valueAxis);

    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);

    return chartView;
}

QMap<QString, QPair<int, int>> StatisticsDialog::calculateConfidenceDistribution(const QString &defectType) const {
    QMap<QString, QPair<int, int>> distribution;

    QVector<float> confidences = statistics.defectConfidences.value(defectType);
    if (confidences.isEmpty()) {
        return distribution;
    }

    // 定义置信度区间
    distribution["0.0-0.5"] = qMakePair(0, 0);
    distribution["0.5-0.6"] = qMakePair(0, 0);
    distribution["0.6-0.7"] = qMakePair(0, 0);
    distribution["0.7-0.8"] = qMakePair(0, 0);
    distribution["0.8-0.9"] = qMakePair(0, 0);
    distribution["0.9-1.0"] = qMakePair(0, 0);

    for (float confidence : confidences) {
        if (confidence < 0.5) {
            distribution["0.0-0.5"].first++;
            distribution["0.0-0.5"].second++;
        } else if (confidence < 0.6) {
            distribution["0.5-0.6"].first++;
            distribution["0.5-0.6"].second++;
        } else if (confidence < 0.7) {
            distribution["0.6-0.7"].first++;
            distribution["0.6-0.7"].second++;
        } else if (confidence < 0.8) {
            distribution["0.7-0.8"].first++;
            distribution["0.7-0.8"].second++;
        } else if (confidence < 0.9) {
            distribution["0.8-0.9"].first++;
            distribution["0.8-0.9"].second++;
        } else {
            distribution["0.9-1.0"].first++;
            distribution["0.9-1.0"].second++;
        }
    }

    return distribution;
}

QMap<QString, QPair<int, int>> StatisticsDialog::calculateTotalConfidenceDistribution(const QVector<float> &allConfidences) const {
    QMap<QString, QPair<int, int>> distribution;

    // 定义置信度区间
    distribution["0.0-0.5"] = qMakePair(0, 0);
    distribution["0.5-0.6"] = qMakePair(0, 0);
    distribution["0.6-0.7"] = qMakePair(0, 0);
    distribution["0.7-0.8"] = qMakePair(0, 0);
    distribution["0.8-0.9"] = qMakePair(0, 0);
    distribution["0.9-1.0"] = qMakePair(0, 0);

    for (float confidence : allConfidences) {
        if (confidence < 0.5) {
            distribution["0.0-0.5"].first++;
            distribution["0.0-0.5"].second++;
        } else if (confidence < 0.6) {
            distribution["0.5-0.6"].first++;
            distribution["0.5-0.6"].second++;
        } else if (confidence < 0.7) {
            distribution["0.6-0.7"].first++;
            distribution["0.6-0.7"].second++;
        } else if (confidence < 0.8) {
            distribution["0.7-0.8"].first++;
            distribution["0.7-0.8"].second++;
        } else if (confidence < 0.9) {
            distribution["0.8-0.9"].first++;
            distribution["0.8-0.9"].second++;
        } else {
            distribution["0.9-1.0"].first++;
            distribution["0.9-1.0"].second++;
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