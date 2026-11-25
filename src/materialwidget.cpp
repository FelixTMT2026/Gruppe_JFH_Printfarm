// Implementierung siehe materialwidget.h.

#include "materialwidget.h"
#include "achseutil.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableView>
#include <QHeaderView>
#include <QGroupBox>
#include <QLabel>
#include <QMap>
#include <QDate>
#include <QFont>
#include <QPainter>
#include <cmath>
#include <QApplication>
#include <QPalette>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QPen>

MaterialWidget::MaterialWidget(DataStore *store, QWidget *parent)
    : QWidget(parent), m_store(store)
{
    auto *layout = new QVBoxLayout(this);

    auto *matBox = new QGroupBox(tr("Materialien"), this);
    auto *ml = new QVBoxLayout(matBox);
    auto *matView = new QTableView(matBox);
    matView->setAlternatingRowColors(true);
    auto *matModel = new MaterialModel(store, matBox);
    connect(store, &DataStore::dataLoaded, matModel, &MaterialModel::refresh);
    matView->setModel(matModel);
    matView->setSortingEnabled(true);
    matView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ml->addWidget(matView);

    auto *statBox = new QGroupBox(tr("Verbrauchsstatistik"), this);
    auto *sl = new QVBoxLayout(statBox);
    m_stats = new QLabel(statBox);
    m_stats->setWordWrap(true);
    m_stats->setMaximumHeight(110);
    sl->addWidget(m_stats);

    m_series = new QLineSeries();
    m_series->setName(tr("Filament [kg]"));
    m_series->setPointsVisible(true);
    QPen pen = m_series->pen();
    pen.setWidth(2);
    m_series->setPen(pen);
    auto *chart = new QChart();
    chart->addSeries(m_series);
    chart->setTitle(tr("Verbrauch im 6-Stunden-Takt (erfolgreiche Drucke)"));
    chart->legend()->setVisible(true);
    chart->setAnimationOptions(QChart::NoAnimation);
    m_axisX = new QDateTimeAxis();
    m_axisX->setTitleText(tr("Tag"));
    m_axisX->setFormat(QStringLiteral("d"));
    QFont axisFont = m_axisX->labelsFont();
    axisFont.setPointSize(8);
    m_axisX->setLabelsFont(axisFont);
    chart->addAxis(m_axisX, Qt::AlignBottom);
    auto *axisY = new QValueAxis();
    axisY->setTitleText(tr("Filament [kg]"));
    axisY->setLabelFormat(QStringLiteral("%.1f"));
    chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(axisY);
    m_chart = new QChartView(chart, statBox);
    m_chart->setRenderHint(QPainter::Antialiasing);
    m_chart->setMinimumHeight(260);
    chart->setTheme(qApp->palette().color(QPalette::Window).lightness() < 128
                    ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    sl->addWidget(m_chart, 1);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(matBox);
    splitter->addWidget(statBox);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setChildrenCollapsible(false);
    layout->addWidget(splitter);

    connect(chart, &QChart::plotAreaChanged, this, [this](const QRectF &) { adjustAxisLabels(); });

    connect(store, &DataStore::dataLoaded, this, &MaterialWidget::updateStatistics);
    updateStatistics();
}

void MaterialWidget::updateStatistics()
{
    const DataStore *store = m_store;
    struct MatStat { int prints = 0, success = 0; double filamentGrams = 0, costEur = 0; };
    QMap<QString, MatStat> proMaterial;
    QMap<QString, int> errorCodes;
    double totalFilament = 0, totalCost = 0;
    int fertig = 0, success = 0;
    for (const auto &a : store->auftraege()) {
        if (a.jobResult == QLatin1String("ausstehend")) continue;
        ++fertig;
        const Material *m = store->material(a.materialId);
        auto &s = proMaterial[a.materialId];
        s.prints++;
        if (!a.errorCode.isEmpty()) errorCodes[a.errorCode]++;
        if (a.jobResult == QLatin1String("erfolgreich")) {
            ++success;
            s.success++;
            s.filamentGrams += a.plannedFilamentG;
            totalFilament += a.plannedFilamentG;
            if (m) {
                const double k = a.plannedFilamentG / 1000.0 * m->priceEurKg;
                s.costEur += k;
                totalCost += k;
            }
        }
    }

    QString s = tr("<b>Verbrauchsstatistik:</b> Filamentverbrauch gesamt: %1 kg, Materialkosten gesamt: %2 € | "
                   "%3 fertige Drucke, davon %4 erfolgreich (%5 %)")
                    .arg(totalFilament / 1000.0, 0, 'f', 2).arg(totalCost, 0, 'f', 2)
                    .arg(fertig).arg(success)
                    .arg(fertig ? 100.0 * success / fertig : 0.0, 0, 'f', 0);
    s += QStringLiteral("<br><b>") + tr("Pro Material:") + QStringLiteral("</b><br>");
    for (auto it = proMaterial.begin(); it != proMaterial.end(); ++it) {
        const MatStat &v = it.value();
        s += QStringLiteral("%1: %2 Drucke, %3 kg, %4 €, Erfolgsquote %5 %6<br>")
                 .arg(it.key().toHtmlEscaped())
                 .arg(v.prints)
                 .arg(v.filamentGrams / 1000.0, 0, 'f', 2)
                 .arg(v.costEur, 0, 'f', 2)
                 .arg(v.prints ? 100.0 * v.success / v.prints : 0.0, 0, 'f', 0)
                 .arg(QLatin1Char('%'));
    }
    s += QStringLiteral("<b>") + tr("Fehlercodes:") + QStringLiteral("</b> ");
    for (auto it = errorCodes.begin(); it != errorCodes.end(); ++it)
        s += QStringLiteral("%1 (%2) ").arg(it.key().toHtmlEscaped()).arg(it.value());
    m_stats->setText(s);

    QMap<QDateTime, double> proBucket;
    for (const auto &a : store->auftraege()) {
        if (a.jobResult != QLatin1String("erfolgreich") || !a.start.isValid()) continue;
        QDateTime bucket = a.start;
        bucket.setTime(QTime((a.start.time().hour() / 6) * 6, 0, 0));
        proBucket[bucket] += a.plannedFilamentG / 1000.0;
    }
    m_series->clear();
    double maxKg = 0.0;
    QDateTime first, last;
    for (auto it = proBucket.begin(); it != proBucket.end(); ++it) {
        m_series->append(QPointF(it.key().toMSecsSinceEpoch(), it.value()));
        maxKg = qMax(maxKg, it.value());
        if (!first.isValid() || it.key() < first) first = it.key();
        if (!last.isValid() || it.key() > last) last = it.key();
    }
    if (first.isValid()) {
        m_anzahlTage = int(first.date().daysTo(last.date())) + 1;
        m_axisX->setRange(first, last.addSecs(6 * 3600));
        m_axisX->setTickCount(qMax(2, m_anzahlTage));
    }
    adjustAxisLabels();

    if (auto *axisY = qobject_cast<QValueAxis*>(m_chart->chart()->axes(Qt::Vertical).first()))
        axisY->setRange(0.0, qMax(1.0, maxKg * 1.15));
}

void MaterialWidget::adjustAxisLabels()
{

    const QRectF pa = m_chart->chart()->plotArea();
    if (m_anzahlTage > 0)

        m_axisX->setTickCount(qBound(2, int(pa.width() / 24.0), m_anzahlTage));
    if (auto *ay = qobject_cast<QValueAxis*>(m_chart->chart()->axes(Qt::Vertical).first()))
        adjustValueAxis(ay, pa.height());
}
