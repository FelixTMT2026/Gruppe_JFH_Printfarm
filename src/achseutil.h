#pragma once
// Gemeinsame Achsen-Optimierung fuer alle Plots des Projekts:
// - Kategorie-Achsen (Tag/Zeit): Labels passend zur Plotbreite ausduennen,
//   damit Qt sie nie zu "..." elidiert (leere Strings / Weglassen verstecken Labels).
// - Werte-Achsen (Temperatur/kg): Tick-Anzahl an der Plothoehe ausrichten.

#include <QBarCategoryAxis>
#include <QCategoryAxis>
#include <QValueAxis>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QtMath>
#include <cmath>

inline int achsenLabelSchritt(int anzahl, qreal plotBreite, qreal pxProLabel)
{
    // Qt elidiert Labels erst ab ~20-23 px pro Label -> konservativ 24 px annehmen
    return qMax(1, int(std::ceil(anzahl * pxProLabel / qMax(qreal(1.0), plotBreite))));
}

// QBarCategoryAxis: setCategories mit ausgeduennten Labels (leere Strings = versteckt)
inline void kategorieAchseAnpassen(QBarCategoryAxis *achse, const QStringList &alleLabels,
                                   qreal plotBreite, qreal pxProLabel = 24.0)
{
    const int n = alleLabels.size();
    if (!achse || n == 0) return;
    const int step = achsenLabelSchritt(n, plotBreite, pxProLabel);
    QStringList cats;
    cats.reserve(n);
    for (int i = 0; i < n; ++i)
        cats.append(i % step == 0 ? alleLabels.at(i) : QString());
    achse->setCategories(cats);
}

// QCategoryAxis (mit Positionen, z. B. Zeitachse im PlotWidget): leeren + ausgeduennt neu aufbauen
inline void kategorieAchseAnpassen(QCategoryAxis *achse, const QList<QPair<QString, double>> &alleLabels,
                                   qreal plotBreite, qreal pxProLabel = 24.0)
{
    const int n = alleLabels.size();
    if (!achse || n == 0) return;
    const int step = achsenLabelSchritt(n, plotBreite, pxProLabel);
    const QStringList alt = achse->categoriesLabels();
    for (const auto &c : alt)
        achse->remove(c);
    for (int i = 0; i < n; ++i)
        if (i % step == 0)
            achse->append(alleLabels.at(i).first, alleLabels.at(i).second);
}

// Werte-Achse: moeglichst dicht, aber ohne Ueberlappung, gemessen: ab ~26 px
// Abstand pro Tick werden die Labels nicht mehr zu "..." elidiert
inline void werteAchseAnpassen(QValueAxis *achse, qreal plotHoehe)
{
    if (achse)
        achse->setTickCount(qMax(2, int(plotHoehe / 26.0)));
}
