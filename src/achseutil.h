// Hilfsfunktionen, damit Achsenlabels nie zu "..." elidiert werden.
#pragma once

#include <QBarCategoryAxis>
#include <QCategoryAxis>
#include <QValueAxis>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QtMath>
#include <cmath>

inline int achsenLabelSchritt(int anzahl, qreal plotWidth, qreal pxProLabel)
{

    return qMax(1, int(std::ceil(anzahl * pxProLabel / qMax(qreal(1.0), plotWidth))));
}

inline void adjustCategoryAxis(QBarCategoryAxis *achse, const QStringList &alleLabels,
                                   qreal plotWidth, qreal pxProLabel = 24.0)
{
    const int n = alleLabels.size();
    if (!achse || n == 0) return;
    const int step = achsenLabelSchritt(n, plotWidth, pxProLabel);
    QStringList cats;
    cats.reserve(n);
    for (int i = 0; i < n; ++i)
        cats.append(i % step == 0 ? alleLabels.at(i) : QString());
    achse->setCategories(cats);
}

inline void adjustCategoryAxis(QCategoryAxis *achse, const QList<QPair<QString, double>> &alleLabels,
                                   qreal plotWidth, qreal pxProLabel = 24.0)
{
    const int n = alleLabels.size();
    if (!achse || n == 0) return;
    const int step = achsenLabelSchritt(n, plotWidth, pxProLabel);
    const QStringList original = achse->categoriesLabels();
    for (const auto &c : original)
        achse->remove(c);
    for (int i = 0; i < n; ++i)
        if (i % step == 0)
            achse->append(alleLabels.at(i).first, alleLabels.at(i).second);
}

inline void adjustValueAxis(QValueAxis *achse, qreal plotHeight)
{
    if (achse)
        achse->setTickCount(qMax(2, int(plotHeight / 26.0)));
}
