#include "plotwidget.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QApplication>
#include <QPalette>
#include <QCheckBox>
#include <QCategoryAxis>

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

// Adaptive X-Achsen-Skalierung: die reale Zeit wird non-linear auf die Anzeige-X
// abgebildet. Maßgeblich ist die lokale Änderung der Düsentemperatur: starke
// Änderungen (Sprünge, Aufheizen) werden bis zu 10x gestreckt, stabile Phasen
// normal. Im Standard-Modus (adaptiv=false) bleibt die Achse linear.
QVector<double> PlotWidget::adaptiveZeitachse(const QVector<QPointF> &pts, bool adaptiv)
{
    const int n = pts.size();
    QVector<double> dispX(n);
    if (!adaptiv || n <= 2) {
        for (int i = 0; i < n; ++i)
            dispX[i] = pts[i].x();
        return dispX;
    }
    QVector<double> imp(n, 1e-3);
    double maxImp = 1e-9;
    for (int i = 1; i < n; ++i) {
        imp[i] = qAbs(pts[i].y() - pts[i - 1].y()) + 1e-3;
        maxImp = qMax(maxImp, imp[i]);
    }
    dispX[0] = pts[0].x();
    for (int i = 1; i < n; ++i) {
        const double dx = pts[i].x() - pts[i - 1].x();
        dispX[i] = dispX[i - 1] + dx * (1.0 + 9.0 * imp[i] / maxImp);   // bis 10x gestreckt
    }
    return dispX;
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

    // ALLE Punkte bleiben erhalten; adaptiv wird nur die X-Achsen-Skalierung
    // (Zeit non-linear gestreckt an Änderungsstellen), Standard = linear.
    const QVector<double> dispX = adaptiveZeitachse(m_duesePts, m_adaptiv);

    auto mk = [](const QString &name, const QColor &c) {
        auto *s = new QLineSeries();
        s->setName(name);
        s->setColor(c);
        return s;
    };
    auto *sDuese = mk(tr("Düsentemperatur"), QColor(214, 69, 49));
    auto *sBett = mk(tr("Betttemperatur"), QColor(41, 128, 185));
    for (int i = 0; i < m_duesePts.size(); ++i)
        sDuese->append(dispX[i], m_duesePts[i].y());
    for (int i = 0; i < m_bettPts.size(); ++i)
        sBett->append(dispX[i], m_bettPts[i].y());
    // Ereignis-Marker (nur Pause/Fortsetzen/Abbruch), auf Anzeige-X gemappt
    for (const auto &ev : m_events) {
        double xd = ev.first;
        if (m_adaptiv && !m_duesePts.isEmpty()) {
            for (int i = 1; i < m_duesePts.size(); ++i) {
                if (m_duesePts[i].x() >= ev.first) {
                    const double f = (ev.first - m_duesePts[i - 1].x()) /
                                     qMax(1e-9, m_duesePts[i].x() - m_duesePts[i - 1].x());
                    xd = dispX[i - 1] + f * (dispX[i] - dispX[i - 1]);
                    break;
                }
            }
        }
        auto *s = mk(ev.second, Qt::gray);
        s->append(xd, 0);
        s->append(xd, m_maxTemp);
        s->setPen(QPen(Qt::gray, 0, Qt::DashLine));
        chart()->addSeries(s);
    }
    chart()->addSeries(sDuese);
    chart()->addSeries(sBett);

    // Achsen: X als Kategorieachse, damit trotz verzerrter Skalierung die echte
    // Zeit (min, ganzzahlig) abgelesen werden kann; Y linear ganzzahlig.
    const auto oldAxes = chart()->axes();
    for (auto *a : oldAxes) { chart()->removeAxis(a); delete a; }
    auto *cat = new QCategoryAxis();
    cat->setTitleText(tr("Zeit [min]"));
    const double xMin = dispX.first(), xMax = qMax(dispX.last(), dispX.first() + 1e-9);
    cat->setRange(xMin, xMax);
    const int ticks = 6;
    for (int t = 0; t <= ticks; ++t) {
        const double pos = xMin + (xMax - xMin) * t / ticks;
        double realMin = m_duesePts.last().x();
        for (int i = 0; i < dispX.size(); ++i)
            if (dispX[i] >= pos) { realMin = m_duesePts[i].x(); break; }
        cat->append(QString::number(qRound64(realMin)), pos);
    }
    chart()->addAxis(cat, Qt::AlignBottom);
    auto *ay = new QValueAxis();
    ay->setTitleText(tr("Temperatur [°C]"));
    ay->setLabelFormat(QStringLiteral("%.0f"));
    chart()->addAxis(ay, Qt::AlignLeft);
    for (auto *s : chart()->series()) {
        s->attachAxis(cat);
        s->attachAxis(ay);
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
        for (const auto &pt : pts)   // Vergleich: alle Punkte, lineare Zeit
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
