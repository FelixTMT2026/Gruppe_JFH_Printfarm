#include "plotwidget.h"
#include <QChart>
#include <QValueAxis>
#include <QLineSeries>

PlotWidget::PlotWidget(QWidget *parent) : QChartView(new QChart(), parent)
{
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->legend()->setVisible(true);
    chart()->setTitle(tr("Temperaturverlauf"));
    setRenderHint(QPainter::Antialiasing);
}

void PlotWidget::leeren()
{
    chart()->removeAllSeries();
    chart()->setTitle(tr("Kein Protokoll ausgewählt"));
}

void PlotWidget::zeigeProtokoll(const Protokoll &p)
{
    chart()->removeAllSeries();
    chart()->setTitle(tr("Temperaturverlauf Auftrag %1").arg(p.auftragId));

    auto mk = [](const QString &name, const QColor &c) {
        auto *s = new QLineSeries();
        s->setName(name);
        s->setColor(c);
        return s;
    };
    auto *sSoll = mk(tr("Düse Soll"), Qt::red);
    auto *sIst = mk(tr("Düse Ist"), Qt::darkRed);
    auto *sBett = mk(tr("Bett Ist"), Qt::blue);
    auto *sIstAuf = mk(tr("Düse Ist (Aufheizen)"), QColor(255, 128, 0));
    auto *sBettAuf = mk(tr("Bett Ist (Aufheizen)"), QColor(0, 128, 255));

    for (const auto &m : p.aufheizen) {
        sIstAuf->append(m.zeitS, m.dueseIstC);
        sBettAuf->append(m.zeitS, m.bettIstC);
    }
    for (const auto &m : p.druck) {
        if (!qFuzzyIsNull(m.dueseSollC))
            sSoll->append(m.zeitS, m.dueseSollC);
        sIst->append(m.zeitS, m.dueseIstC);
        sBett->append(m.zeitS, m.bettIstC);
    }
    // Ereignis-Markierungen als senkrechte Linien
    for (const auto &ev : p.ereignisse) {
        if (ev.typ == QLatin1String("druck_start") || ev.typ == QLatin1String("pause") ||
            ev.typ == QLatin1String("druck_ende")) {
            auto *s = mk(ev.typ, Qt::gray);
            double tmax = 0;
            for (const auto &m : p.druck) tmax = qMax(tmax, m.zeitS);
            s->append(ev.zeitS, 0);
            s->append(ev.zeitS, qMax(p.dueseSollC, 250.0));
            s->setPen(QPen(Qt::gray, 0, Qt::DashLine));
            chart()->addSeries(s);
        }
    }
    chart()->addSeries(sSoll);
    chart()->addSeries(sIst);
    chart()->addSeries(sBett);
    chart()->addSeries(sIstAuf);
    chart()->addSeries(sBettAuf);

    chart()->createDefaultAxes();
    auto *ax = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (ax) ax->setTitleText(tr("Zeit [s]"));
    auto *ay = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (ay) ay->setTitleText(tr("Temperatur [°C]"));
}

void PlotWidget::zeigeVergleich(const QList<QPair<QString, QVector<Messpunkt>>> &kurven,
                                bool nurDueseIst)
{
    chart()->removeAllSeries();
    chart()->setTitle(tr("Vergleich Sprungantworten"));
    static const QColor farben[] = {Qt::red, Qt::blue, Qt::green, Qt::magenta, Qt::darkCyan,
                                    Qt::darkYellow, Qt::darkGreen, Qt::darkBlue};
    int i = 0;
    for (const auto &k : kurven) {
        auto *s = new QLineSeries();
        s->setName(k.first);
        s->setColor(farben[i++ % 8]);
        for (const auto &m : k.second)
            s->append(m.zeitS, nurDueseIst ? m.dueseIstC : m.bettIstC);
        chart()->addSeries(s);
    }
    chart()->createDefaultAxes();
    auto *ax = qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).first());
    if (ax) ax->setTitleText(tr("Zeit [s]"));
    auto *ay = qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());
    if (ay) ay->setTitleText(tr("Temperatur [°C]"));
}
