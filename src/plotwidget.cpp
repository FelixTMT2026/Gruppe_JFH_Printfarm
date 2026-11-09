#include "plotwidget.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QApplication>
#include <QPalette>

PlotWidget::PlotWidget(QWidget *parent) : QChartView(new QChart(), parent)
{
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom | Qt::AlignLeft);   // Legende unten links
    chart()->setTitle(QString());   // keine Überschrift
    // Chart-Theme folgt dem App-Theme (hell/dunkel)
    setDarkMode(qApp->palette().color(QPalette::Window).lightness() < 128);
    setRenderHint(QPainter::Antialiasing);
}

void PlotWidget::setDarkMode(bool dark)
{
    chart()->setTheme(dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
}

QVector<QPointF> PlotWidget::lttb(const QVector<QPointF> &points, int threshold)
{
    // Largest-Triangle-Three-Buckets: wählt pro Bucket den Punkt mit der größten
    // Dreiecksfläche (Abweichung) -> starke Änderungen bleiben detailreich,
    // stabile Bereiche werden auf wenige Punkte reduziert.
    const int n = points.size();
    if (threshold <= 0 || n <= threshold)
        return points;
    QVector<QPointF> out;
    out.reserve(threshold);
    out.append(points.first());
    const int bucketSize = (n - 2) / (threshold - 2);
    for (int i = 0; i < threshold - 2; ++i) {
        const int b0 = 1 + i * bucketSize;
        const int b1 = qMin(n - 1, b0 + bucketSize);
        // durchschnittlicher nächster Bucket
        double avgX = 0, avgY = 0;
        const int n0 = b1, n1 = qMin(n - 1, b1 + bucketSize);
        for (int j = n0; j < n1; ++j) { avgX += points[j].x(); avgY += points[j].y(); }
        const int cnt = qMax(1, n1 - n0);
        avgX /= cnt; avgY /= cnt;
        const QPointF a = points[b0 - 1];
        const QPointF avg(avgX, avgY);
        double maxArea = -1;
        int maxIdx = b0;
        for (int j = b0; j < b1; ++j) {
            const QPointF p = points[j];
            const double area = qAbs((a.x() - avg.x()) * (p.y() - a.y()) -
                                     (a.x() - p.x()) * (avg.y() - a.y())) * 0.5;
            if (area > maxArea) { maxArea = area; maxIdx = j; }
        }
        out.append(points[maxIdx]);
    }
    out.append(points.last());
    return out;
}

void PlotWidget::leeren()
{
    chart()->removeAllSeries();
    m_auftragId.clear();
}

void PlotWidget::zeigeProtokoll(const Protokoll &p)
{
    chart()->removeAllSeries();
    chart()->setTitle(QString());   // keine Überschrift
    m_auftragId = p.auftragId;

    auto mk = [](const QString &name, const QColor &c) {
        auto *s = new QLineSeries();
        s->setName(name);
        s->setColor(c);
        return s;
    };
    // nur echte Messdaten: Düsentemperatur und Betttemperatur über die ganze Zeit
    auto *sDuese = mk(tr("Düsentemperatur"), QColor(214, 69, 49));
    auto *sBett = mk(tr("Betttemperatur"), QColor(41, 128, 185));

    // Zeitachse in Minuten (Protokoll intern in Sekunden); adaptive Auflösung:
    // LTTB behält an Sprüngen/Schwankungen die Detailpunkte, streicht stabile Phasen
    QVector<QPointF> duesePts, bettPts;
    for (const auto &m : p.aufheizen) {
        duesePts.append({m.zeitS / 60.0, m.dueseIstC});
        bettPts.append({m.zeitS / 60.0, m.bettIstC});
    }
    for (const auto &m : p.druck) {
        duesePts.append({m.zeitS / 60.0, m.dueseIstC});
        bettPts.append({m.zeitS / 60.0, m.bettIstC});
    }
    const int schwelle = qMax(200, width());   // ca. ein Punkt pro Pixel
    for (const auto &pt : lttb(duesePts, schwelle))
        sDuese->append(pt);
    for (const auto &pt : lttb(bettPts, schwelle))
        sBett->append(pt);
    // Ereignis-Markierungen als senkrechte Linien (nur Pause/Abbruch, nicht Start/Ende)
    for (const auto &ev : p.ereignisse) {
        if (ev.typ == QLatin1String("pause") || ev.typ == QLatin1String("fortsetzen") ||
            ev.typ == QLatin1String("abbruch")) {
            QString evName;
            if (ev.typ == QLatin1String("druck_start")) evName = tr("Druckstart");
            else if (ev.typ == QLatin1String("pause")) evName = tr("Pause");
            else if (ev.typ == QLatin1String("fortsetzen")) evName = tr("Fortsetzen");
            else if (ev.typ == QLatin1String("abbruch")) evName = tr("Abbruch");
            else evName = tr("Druckende");
            auto *s = mk(evName, Qt::gray);
            s->append(ev.zeitS / 60.0, 0);
            s->append(ev.zeitS / 60.0, qMax(p.dueseSollC, 250.0));
            s->setPen(QPen(Qt::gray, 0, Qt::DashLine));
            chart()->addSeries(s);
        }
    }
    chart()->addSeries(sDuese);
    chart()->addSeries(sBett);

    chart()->createDefaultAxes();
    auto *ax = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (ax) {
        ax->setTitleText(tr("Zeit [min]"));
        ax->setLabelFormat(QStringLiteral("%.0f"));   // nur ganze Zahlen auf der Achse
    }
    auto *ay = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (ay) {
        ay->setTitleText(tr("Temperatur [°C]"));
        ay->setLabelFormat(QStringLiteral("%.0f"));   // nur ganze Zahlen auf der Achse
    }
}

void PlotWidget::zeigeVergleich(const QList<QPair<QString, QVector<Messpunkt>>> &kurven,
                                bool nurDueseIst)
{
    chart()->removeAllSeries();
    chart()->setTitle(QString());
    static const QColor farben[] = {Qt::red, Qt::blue, Qt::green, Qt::magenta, Qt::darkCyan,
                                    Qt::darkYellow, Qt::darkGreen, Qt::darkBlue};
    int i = 0;
    for (const auto &k : kurven) {
        auto *s = new QLineSeries();
        s->setName(k.first);
        s->setColor(farben[i++ % 8]);
        QVector<QPointF> pts;
        for (const auto &m : k.second)
            pts.append({m.zeitS / 60.0, nurDueseIst ? m.dueseIstC : m.bettIstC});
        for (const auto &pt : lttb(pts, qMax(200, width())))
            s->append(pt);
        chart()->addSeries(s);
    }
    chart()->createDefaultAxes();
    auto *ax = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (ax) {
        ax->setTitleText(tr("Zeit [min]"));
        ax->setLabelFormat(QStringLiteral("%.0f"));
    }
    auto *ay = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (ay) {
        ay->setTitleText(tr("Temperatur [°C]"));
        ay->setLabelFormat(QStringLiteral("%.0f"));
    }
}
