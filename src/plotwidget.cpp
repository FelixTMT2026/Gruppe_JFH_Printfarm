#include "plotwidget.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QApplication>
#include <QPalette>
#include <QCheckBox>
#include <climits>

PlotWidget::PlotWidget(QWidget *parent) : QChartView(new QChart(), parent)
{
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom | Qt::AlignLeft);   // Legende unten links
    chart()->setTitle(QString());                                       // keine Überschrift
    setDarkMode(qApp->palette().color(QPalette::Window).lightness() < 128);
    setRenderHint(QPainter::Antialiasing);

    // Toggle oben rechts: adaptiv (LTTB-Decimation) vs. Standard (alle Punkte)
    m_adaptivBox = new QCheckBox(tr("Adaptiv"), this);
    m_adaptivBox->setChecked(true);
    m_adaptivBox->setToolTip(tr("Adaptive Auflösung (LTTB) umschalten"));
    connect(m_adaptivBox, &QCheckBox::toggled, this, [this](bool on) {
        m_adaptiv = on;
        render();
    });
}

void PlotWidget::setDarkMode(bool dark)
{
    chart()->setTheme(dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
}

// LTTB (Steinarsson): pro Bucket wird der Punkt mit der größten Dreiecksfläche
// gewählt — visuell redundante Punkte (gerade/stabile Bereiche) werden entfernt,
// Sprünge und Maxima bleiben erhalten. Zeitachse bleibt linear.
QVector<QPointF> PlotWidget::lttb(const QVector<QPointF> &points, int threshold)
{
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
        double avgX = 0, avgY = 0;
        const int n1 = qMin(n, b1 + bucketSize);
        for (int j = b1; j < n1; ++j) { avgX += points[j].x(); avgY += points[j].y(); }
        const int cnt = qMax(1, n1 - b1);
        const QPointF avg(avgX / cnt, avgY / cnt);
        const QPointF a = points[b0 - 1];
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
    m_rohdaten = false;
    m_duesePts.clear();
    m_bettPts.clear();
    m_events.clear();
}

void PlotWidget::zeigeProtokoll(const Protokoll &p)
{
    // Rohdaten (Zeit in min) speichern; Zeichnen passiert in render(),
    // das bei Resize/Toggle mit den aktuellen Parametern neu rendert.
    m_auftragId = p.auftragId;
    m_duesePts.clear();
    m_bettPts.clear();
    m_events.clear();
    for (const auto &m : p.aufheizen) {
        m_duesePts.append({m.zeitS / 60.0, m.dueseIstC});
        m_bettPts.append({m.zeitS / 60.0, m.bettIstC});
    }
    for (const auto &m : p.druck) {
        m_duesePts.append({m.zeitS / 60.0, m.dueseIstC});
        m_bettPts.append({m.zeitS / 60.0, m.bettIstC});
    }
    m_maxTemp = qMax(p.dueseSollC, 250.0);
    for (const auto &pt : m_duesePts) m_maxTemp = qMax(m_maxTemp, pt.y());
    for (const auto &ev : p.ereignisse) {
        if (ev.typ == QLatin1String("pause") || ev.typ == QLatin1String("fortsetzen") ||
            ev.typ == QLatin1String("abbruch")) {
            QString evName;
            if (ev.typ == QLatin1String("pause")) evName = tr("Pause");
            else if (ev.typ == QLatin1String("fortsetzen")) evName = tr("Fortsetzen");
            else evName = tr("Abbruch");
            m_events.append({ev.zeitS / 60.0, evName});
        }
    }
    m_rohdaten = true;
    render();
}

void PlotWidget::render()
{
    chart()->removeAllSeries();
    chart()->setTitle(QString());
    if (!m_rohdaten)
        return;

    // Zeitachse bleibt linear (in Minuten); adaptiv = LTTB-Decimation auf
    // die aktuelle Plot-Breite (~1 Punkt/Pixel), Standard = alle Punkte.
    const int schwelle = m_adaptiv ? qMax(200, width()) : INT_MAX;

    auto mk = [](const QString &name, const QColor &c) {
        auto *s = new QLineSeries();
        s->setName(name);
        s->setColor(c);
        return s;
    };
    auto *sDuese = mk(tr("Düsentemperatur"), QColor(214, 69, 49));
    auto *sBett = mk(tr("Betttemperatur"), QColor(41, 128, 185));
    for (const auto &pt : lttb(m_duesePts, schwelle))
        sDuese->append(pt);
    for (const auto &pt : lttb(m_bettPts, schwelle))
        sBett->append(pt);
    // Ereignis-Marker (nur Pause/Fortsetzen/Abbruch)
    for (const auto &ev : m_events) {
        auto *s = mk(ev.second, Qt::gray);
        s->append(ev.first, 0);
        s->append(ev.first, m_maxTemp);
        s->setPen(QPen(Qt::gray, 0, Qt::DashLine));
        chart()->addSeries(s);
    }
    chart()->addSeries(sDuese);
    chart()->addSeries(sBett);

    chart()->createDefaultAxes();
    auto *ax = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (ax) {
        ax->setTitleText(tr("Zeit [min]"));
        ax->setLabelFormat(QStringLiteral("%.0f"));   // nur ganze Zahlen
    }
    auto *ay = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (ay) {
        ay->setTitleText(tr("Temperatur [°C]"));
        ay->setLabelFormat(QStringLiteral("%.0f"));   // nur ganze Zahlen
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
        for (const auto &pt : lttb(pts, m_adaptiv ? qMax(200, width()) : INT_MAX))
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

void PlotWidget::resizeEvent(QResizeEvent *e)
{
    QChartView::resizeEvent(e);
    if (m_adaptivBox)
        m_adaptivBox->move(width() - m_adaptivBox->width() - 8, 8);   // Toggle oben rechts
    if (m_rohdaten)
        render();   // adaptive Auflösung an neue Breite anpassen
}
