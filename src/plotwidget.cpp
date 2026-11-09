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

    // Zeitachse in Minuten (Protokoll intern in Sekunden)
    for (const auto &m : p.aufheizen) {
        sDuese->append(m.zeitS / 60.0, m.dueseIstC);
        sBett->append(m.zeitS / 60.0, m.bettIstC);
    }
    for (const auto &m : p.druck) {
        sDuese->append(m.zeitS / 60.0, m.dueseIstC);
        sBett->append(m.zeitS / 60.0, m.bettIstC);
    }
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
        for (const auto &m : k.second)
            s->append(m.zeitS / 60.0, nurDueseIst ? m.dueseIstC : m.bettIstC);
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
