// Implementierung siehe plotwidget.h.

#include "plotwidget.h"
#include "achseutil.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>
#include <QApplication>
#include <QPalette>
#include <QCheckBox>
#include <QCategoryAxis>

PlotWidget::PlotWidget(QWidget *parent) : QChartView(new QChart(), parent)
{

    setMinimumHeight(180);
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    chart()->setTitle(QString());
    setDarkMode(qApp->palette().color(QPalette::Window).lightness() < 128);
    setRenderHint(QPainter::Antialiasing);

    m_adaptiveBox = new QCheckBox(tr("Adaptiv"), this);
    m_adaptiveBox->setChecked(false);
    m_adaptiveBox->setToolTip(tr("Adaptive X-Achsen-Skalierung umschalten"));
    connect(m_adaptiveBox, &QCheckBox::toggled, this, [this](bool on) {
        m_adaptive = on;
        drawChart();
    });

    connect(chart(), &QChart::plotAreaChanged, this, [this](const QRectF &) { adjustAxes(); });
}

void PlotWidget::adjustAxes()
{
    const QRectF pa = chart()->plotArea();
    for (auto *a : chart()->axes(Qt::Horizontal)) {
        if (auto *cat = qobject_cast<QCategoryAxis*>(a))
            adjustCategoryAxis(cat, m_xLabels, pa.width());
        else if (auto *val = qobject_cast<QValueAxis*>(a))
            adjustValueAxis(val, pa.height());
    }
    for (auto *a : chart()->axes(Qt::Vertical))
        if (auto *val = qobject_cast<QValueAxis*>(a))
            adjustValueAxis(val, pa.height());
}

void PlotWidget::setDarkMode(bool dark)
{
    chart()->setTheme(dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
}

// Optionale nicht-lineare Zeitachse: Bereiche mit starker Aenderung werden
// gestreckt. Standard bleibt linear; alle Punkte bleiben erhalten.
QVector<double> PlotWidget::adaptiveTimeScale(const QVector<QPointF> &pts, bool adaptive)
{
    const int n = pts.size();
    QVector<double> dispX(n);
    if (!adaptive || n <= 2) {
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
        dispX[i] = dispX[i - 1] + dx * (1.0 + 9.0 * imp[i] / maxImp);
    }
    return dispX;
}

void PlotWidget::clear()
{
    chart()->removeAllSeries();
    m_jobId.clear();
    m_rawData = false;
    m_nozzlePts.clear();
    m_bedPts.clear();
    m_events.clear();
}

void PlotWidget::showProtocol(const PrintLog &p)
{

    m_jobId = p.jobId;
    m_nozzlePts.clear();
    m_bedPts.clear();
    m_events.clear();
    for (const auto &m : p.heatUp) {
        m_nozzlePts.append({m.timeS / 60.0, m.nozzleActualC});
        m_bedPts.append({m.timeS / 60.0, m.bedActualC});
    }
    for (const auto &m : p.printPhase) {
        m_nozzlePts.append({m.timeS / 60.0, m.nozzleActualC});
        m_bedPts.append({m.timeS / 60.0, m.bedActualC});
    }
    m_maxTemp = qMax(p.nozzleTargetC, 250.0);
    for (const auto &pt : m_nozzlePts) m_maxTemp = qMax(m_maxTemp, pt.y());
    for (const auto &ev : p.events) {
        if (ev.type == QLatin1String("pause") || ev.type == QLatin1String("fortsetzen") ||
            ev.type == QLatin1String("abbruch")) {
            QString evName;
            if (ev.type == QLatin1String("pause")) evName = tr("Pause");
            else if (ev.type == QLatin1String("fortsetzen")) evName = tr("Fortsetzen");
            else evName = tr("Abbruch");
            m_events.append({ev.timeS / 60.0, evName});
        }
    }
    m_rawData = true;
    drawChart();
}

void PlotWidget::drawChart()
{
    chart()->removeAllSeries();
    chart()->setTitle(QString());
    if (!m_rawData)
        return;

    const QVector<double> dispX = adaptiveTimeScale(m_nozzlePts, m_adaptive);

    auto mk = [](const QString &name, const QColor &c) {
        auto *s = new QLineSeries();
        s->setName(name);
        s->setColor(c);
        return s;
    };
    auto *nozzleSeries = mk(tr("Düsentemperatur"), QColor(214, 69, 49));
    auto *bedSeries = mk(tr("Betttemperatur"), QColor(41, 128, 185));
    for (int i = 0; i < m_nozzlePts.size(); ++i)
        nozzleSeries->append(dispX[i], m_nozzlePts[i].y());
    for (int i = 0; i < m_bedPts.size(); ++i)
        bedSeries->append(dispX[i], m_bedPts[i].y());

    for (const auto &ev : m_events) {
        double xd = ev.first;
        if (m_adaptive && !m_nozzlePts.isEmpty()) {
            for (int i = 1; i < m_nozzlePts.size(); ++i) {
                if (m_nozzlePts[i].x() >= ev.first) {
                    const double f = (ev.first - m_nozzlePts[i - 1].x()) /
                                     qMax(1e-9, m_nozzlePts[i].x() - m_nozzlePts[i - 1].x());
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
    chart()->addSeries(nozzleSeries);
    chart()->addSeries(bedSeries);

    const auto oldAxes = chart()->axes();
    for (auto *a : oldAxes) { chart()->removeAxis(a); delete a; }
    auto *cat = new QCategoryAxis();
    cat->setTitleText(tr("Zeit [min]"));
    const double xMin = dispX.first(), xMax = qMax(dispX.last(), dispX.first() + 1e-9);
    cat->setRange(xMin, xMax);
    const int ticks = 6;
    m_xLabels.clear();
    for (int t = 0; t <= ticks; ++t) {
        const double pos = xMin + (xMax - xMin) * t / ticks;
        double realMin = m_nozzlePts.last().x();
        for (int i = 0; i < dispX.size(); ++i)
            if (dispX[i] >= pos) { realMin = m_nozzlePts[i].x(); break; }
        const QString label = QString::number(qRound64(realMin));
        m_xLabels.append({label, pos});
        cat->append(label, pos);
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
    adjustAxes();
}

void PlotWidget::showComparison(const QList<QPair<QString, QVector<Sample>>> &curves,
                                bool onlyNozzleActual)
{
    chart()->removeAllSeries();
    chart()->setTitle(QString());
    static const QColor farben[] = {Qt::red, Qt::blue, Qt::green, Qt::magenta, Qt::darkCyan,
                                    Qt::darkYellow, Qt::darkGreen, Qt::darkBlue};
    int i = 0;
    for (const auto &k : curves) {
        auto *s = new QLineSeries();
        s->setName(k.first);
        s->setColor(farben[i++ % 8]);
        QVector<QPointF> pts;
        for (const auto &m : k.second)
            pts.append({m.timeS / 60.0, onlyNozzleActual ? m.nozzleActualC : m.bedActualC});
        for (const auto &pt : pts)
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
    adjustAxes();
}

void PlotWidget::resizeEvent(QResizeEvent *e)
{
    QChartView::resizeEvent(e);
    if (m_adaptiveBox)
        m_adaptiveBox->move(width() - m_adaptiveBox->width() - 8, 8);
    if (m_rawData)
        drawChart();
}
