#include "plotwidget.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QApplication>
#include <QPalette>
#include <QCheckBox>
#include <QCategoryAxis>
#include <climits>

PlotWidget::PlotWidget(QWidget *parent) : QChartView(new QChart(), parent)
{
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom | Qt::AlignLeft);   // Legende unten links
    chart()->setTitle(QString());   // keine Überschrift
    // Chart-Theme folgt dem App-Theme (hell/dunkel)
    setDarkMode(qApp->palette().color(QPalette::Window).lightness() < 128);
    setRenderHint(QPainter::Antialiasing);

    // Toggle oben rechts: adaptive (LTTB) vs. Standard (alle Punkte)
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
    m_rohdaten = false;
    m_duesePts.clear();
    m_bettPts.clear();
    m_events.clear();
}

void PlotWidget::resizeEvent(QResizeEvent *e)
{
    QChartView::resizeEvent(e);
    if (m_adaptivBox)
        m_adaptivBox->move(width() - m_adaptivBox->width() - 8, 8);   // Toggle oben rechts halten
    if (m_rohdaten)
        render();   // bei Skalierung mit neuer Breite adaptiv neu decimieren
}

void PlotWidget::zeigeProtokoll(const Protokoll &p)
{
    // Rohdaten (Zeit in Minuten) merken — das eigentliche Zeichnen macht render(),
    // das bei jeder Größenänderung mit der aktuellen Breite neu decimiert.
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
    double maxTemp = p.dueseSollC;
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
    for (const auto &pt : m_duesePts) maxTemp = qMax(maxTemp, pt.y());
    m_maxTemp = qMax(maxTemp, 250.0);
    m_rohdaten = true;
    render();
}

void PlotWidget::render()
{
    chart()->removeAllSeries();
    chart()->setTitle(QString());
    if (!m_rohdaten)
        return;
    auto mk = [](const QString &name, const QColor &c) {
        auto *s = new QLineSeries();
        s->setName(name);
        s->setColor(c);
        return s;
    };
    // X-Warping: im adaptiven Modus wird die Zeitachse anhand der lokalen
    // Temperaturänderung gestreckt (Sprünge/Änderungen breiter, gerade Linien
    // kürzer); im Standard-Modus bleibt die Zeit linear.
    const int n = m_duesePts.size();
    QVector<double> dispX(n);
    if (m_adaptiv && n > 2) {
        QVector<double> imp(n, 1e-3);
        double maxImp = 1e-9;
        for (int i = 1; i < n; ++i) {
            imp[i] = qAbs(m_duesePts[i].y() - m_duesePts[i - 1].y()) + 1e-3;
            maxImp = qMax(maxImp, imp[i]);
        }
        dispX[0] = m_duesePts[0].x();
        for (int i = 1; i < n; ++i) {
            const double dx = m_duesePts[i].x() - m_duesePts[i - 1].x();
            dispX[i] = dispX[i - 1] + dx * (1.0 + 9.0 * imp[i] / maxImp);   // bis 10x gestreckt
        }
    } else {
        for (int i = 0; i < n; ++i)
            dispX[i] = m_duesePts[i].x();
    }
    // Anzeige-Punkte (display-x, y) bilden und decimieren
    QVector<QPointF> dueseDisp, bettDisp;
    for (int i = 0; i < n; ++i) {
        dueseDisp.append({dispX[i], m_duesePts[i].y()});
        bettDisp.append({dispX[i], m_bettPts[i].y()});
    }
    const int schwelle = m_adaptiv ? qMax(200, width()) : INT_MAX;
    auto *sDuese = mk(tr("Düsentemperatur"), QColor(214, 69, 49));
    auto *sBett = mk(tr("Betttemperatur"), QColor(41, 128, 185));
    for (const auto &pt : lttb(dueseDisp, schwelle))
        sDuese->append(pt);
    for (const auto &pt : lttb(bettDisp, schwelle))
        sBett->append(pt);
    // Ereignisse auf die verzerrte Zeitachse mappen (lineare Interpolation)
    for (const auto &ev : m_events) {
        double xd = ev.first;
        if (m_adaptiv && n > 1) {
            for (int i = 1; i < n; ++i) {
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

    // Achsen neu aufbauen: X als Kategorieachse, damit die Beschriftung die echte
    // Zeit (min) an den verzerrten Positionen zeigt; Y ganzzahlig.
    const auto oldAxes = chart()->axes();
    for (auto *a : oldAxes) { chart()->removeAxis(a); delete a; }
    auto *cat = new QCategoryAxis();
    cat->setTitleText(tr("Zeit [min]"));
    const double xMin = dispX.first(), xMax = dispX.last();
    cat->setRange(xMin, qMax(xMax, xMin + 1e-9));
    const int ticks = 6;
    for (int t = 0; t <= ticks; ++t) {
        const double pos = xMin + (xMax - xMin) * t / ticks;
        // echte Zeit an dieser Anzeige-Position finden
        double realMin = m_duesePts.last().x();
        for (int i = 0; i < n; ++i)
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
