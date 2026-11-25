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

    // oben: Liste der Materialien
    auto *matBox = new QGroupBox(tr("Materialien"), this);
    auto *ml = new QVBoxLayout(matBox);
    auto *matView = new QTableView(matBox);
    matView->setAlternatingRowColors(true);   // Zebra-Muster (Farben via Stylesheet)
    auto *matModel = new MaterialModel(store, matBox);
    connect(store, &DataStore::datenGeladen, matModel, &MaterialModel::refresh);
    matView->setModel(matModel);
    matView->setSortingEnabled(true);   // Klick auf Spaltentitel sortiert auf/absteigend
    matView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ml->addWidget(matView);

    // darunter: Verbrauchsstatistik (Erweiterung 1): Text oben, Plot darunter volle Breite
    auto *statBox = new QGroupBox(tr("Verbrauchsstatistik"), this);
    auto *sl = new QVBoxLayout(statBox);
    m_stats = new QLabel(statBox);
    m_stats->setWordWrap(true);
    m_stats->setMaximumHeight(110);   // Plot braucht genug Hoehe, sonst werden Achsenlabels zu "..."
    sl->addWidget(m_stats);

    m_series = new QLineSeries();
    m_series->setName(tr("Filament [kg]"));
    m_series->setPointsVisible(true);   // Kurve mit Punkten, Punkte werden nie ausgeblendet
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
    m_axisX->setFormat(QStringLiteral("d"));   // nur Tageszahl auf der Achse
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
    m_chart->setMinimumHeight(260);   // Plotflaeche muss hoch genug bleiben, sonst "..."-Labels
    chart->setTheme(qApp->palette().color(QPalette::Window).lightness() < 128
                    ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    sl->addWidget(m_chart, 1);

    // Splitter: Grenze zwischen Materialliste und Statistik verschiebbar
    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(matBox);
    splitter->addWidget(statBox);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);   // Statistik-Box groesser als Materialliste
    splitter->setChildrenCollapsible(false);
    layout->addWidget(splitter);

    // bei jeder Groessen-/Layoutaenderung Labels neu anpassen (nie "...")
    connect(chart, &QChart::plotAreaChanged, this, [this](const QRectF &) { aktualisiereAchsenLabels(); });

    connect(store, &DataStore::datenGeladen, this, &MaterialWidget::aktualisiereStats);
    aktualisiereStats();
}

void MaterialWidget::aktualisiereStats()
{
    const DataStore *store = m_store;
    struct MatStat { int drucke = 0, erfolg = 0; double filamentG = 0, kostenEur = 0; };
    QMap<QString, MatStat> proMaterial;
    QMap<QString, int> fehlercodes;
    double filamentGes = 0, kostenGes = 0;
    int fertig = 0, erfolg = 0;
    for (const auto &a : store->auftraege()) {
        if (a.ergebnis == QLatin1String("ausstehend")) continue;
        ++fertig;
        const Material *m = store->material(a.materialId);
        auto &s = proMaterial[a.materialId];
        s.drucke++;
        if (!a.fehlercode.isEmpty()) fehlercodes[a.fehlercode]++;
        if (a.ergebnis == QLatin1String("erfolgreich")) {
            ++erfolg;
            s.erfolg++;
            s.filamentG += a.filamentSollG;
            filamentGes += a.filamentSollG;
            if (m) {
                const double k = a.filamentSollG / 1000.0 * m->preisEurKg;
                s.kostenEur += k;
                kostenGes += k;
            }
        }
    }

    QString s = tr("<b>Verbrauchsstatistik:</b> Filamentverbrauch gesamt: %1 kg, Materialkosten gesamt: %2 € | "
                   "%3 fertige Drucke, davon %4 erfolgreich (%5 %)")
                    .arg(filamentGes / 1000.0, 0, 'f', 2).arg(kostenGes, 0, 'f', 2)
                    .arg(fertig).arg(erfolg)
                    .arg(fertig ? 100.0 * erfolg / fertig : 0.0, 0, 'f', 0);
    s += QStringLiteral("<br><b>") + tr("Pro Material:") + QStringLiteral("</b><br>");
    for (auto it = proMaterial.begin(); it != proMaterial.end(); ++it) {
        const MatStat &v = it.value();
        s += QStringLiteral("%1: %2 Drucke, %3 kg, %4 €, Erfolgsquote %5 %6<br>")
                 .arg(it.key().toHtmlEscaped())
                 .arg(v.drucke)
                 .arg(v.filamentG / 1000.0, 0, 'f', 2)
                 .arg(v.kostenEur, 0, 'f', 2)
                 .arg(v.drucke ? 100.0 * v.erfolg / v.drucke : 0.0, 0, 'f', 0)
                 .arg(QLatin1Char('%'));
    }
    s += QStringLiteral("<b>") + tr("Fehlercodes:") + QStringLiteral("</b> ");
    for (auto it = fehlercodes.begin(); it != fehlercodes.end(); ++it)
        s += QStringLiteral("%1 (%2) ").arg(it.key().toHtmlEscaped()).arg(it.value());
    m_stats->setText(s);

    // Plot: Punkte im 6-Stunden-Takt aufaddiert, ein Punkt pro 6h-Bucket an dessen Beginn
    // (00/06/12/18 Uhr). Alle Originaldaten fliessen ein, beim Komprimieren bleiben alle
    // Punkte erhalten, nur die Achsbeschriftung wird reduziert (aktualisiereAchsenLabels).
    QMap<QDateTime, double> proBucket;
    for (const auto &a : store->auftraege()) {
        if (a.ergebnis != QLatin1String("erfolgreich") || !a.start.isValid()) continue;
        QDateTime bucket = a.start;
        bucket.setTime(QTime((a.start.time().hour() / 6) * 6, 0, 0));
        proBucket[bucket] += a.filamentSollG / 1000.0;
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
    aktualisiereAchsenLabels();
    // Y-Achse explizit an die Daten anpassen (sonst Range 0..0 -> Linie unsichtbar)
    if (auto *axisY = qobject_cast<QValueAxis*>(m_chart->chart()->axes(Qt::Vertical).first()))
        axisY->setRange(0.0, qMax(1.0, maxKg * 1.15));
}

void MaterialWidget::aktualisiereAchsenLabels()
{
    // X (Datum): Tick-Anzahl an Plotbreite - Punkte bleiben alle erhalten, nur Labels werden weniger
    const QRectF pa = m_chart->chart()->plotArea();
    if (m_anzahlTage > 0)
        // ~24 px pro Label, aber hoechstens ein Tick pro Tag (sonst doppelte Tageslabels)
        m_axisX->setTickCount(qBound(2, int(pa.width() / 24.0), m_anzahlTage));
    if (auto *ay = qobject_cast<QValueAxis*>(m_chart->chart()->axes(Qt::Vertical).first()))
        werteAchseAnpassen(ay, pa.height());
}
