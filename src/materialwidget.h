#pragma once
// Tab 3 "Material": Liste der Materialien und darunter eine detaillierte
// Verbrauchsstatistik mit Plot "Verbrauch pro Tag".

#include "datastore.h"
#include "tablemodels.h"
#include <QWidget>

class MaterialWidget : public QWidget {
    Q_OBJECT
public:
    explicit MaterialWidget(DataStore *store, QWidget *parent = nullptr);
private:
    void aktualisiereStats();
    void aktualisiereAchsenLabels();   // Tag-Labels passend zur Plotbreite (nie "...")
    DataStore *m_store;
    class QLabel *m_stats;
    class QChartView *m_chart;
    class QLineSeries *m_series;   // Verbrauch als Kurve mit Punkten (alle Punkte bleiben erhalten)
    class QDateTimeAxis *m_axisX;  // X als Datum (Label nur Tag), Y als kg
    int m_anzahlTage = 0;
};
