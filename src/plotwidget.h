#pragma once
// Liniendiagramm auf Basis von Qt Charts: Temperaturverläufe pro Auftrag.

#include <QChartView>
#include <QLineSeries>
#include "datatypes.h"

class PlotWidget : public QChartView {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    // Zeichnet Aufheiz- und Druckphase: Düse Soll/Ist, Bett Ist.
    void zeigeProtokoll(const Protokoll &p);
    // Zeichnet mehrere simulierte/meessene Sprungantworten (Regleranalyse).
    void zeigeVergleich(const QList<QPair<QString, QVector<Messpunkt>>> &kurven,
                        bool nurDueseIst = true);
    void leeren();
    void setDarkMode(bool dark);   // Chart-Theme an App-Theme koppeln (auch bei Runtime-Wechsel)
};
