// Tab 3 (Material): Materialtabelle + Verbrauchsstatistik mit Plot.
#pragma once

#include "datastore.h"
#include "tablemodels.h"
#include <QWidget>

class MaterialWidget : public QWidget {
    Q_OBJECT
public:
    explicit MaterialWidget(DataStore *store, QWidget *parent = nullptr);
private:
    void updateStatistics();
    void adjustAxisLabels();
    DataStore *m_store;
    class QLabel *m_stats;
    class QChartView *m_chart;
    class QLineSeries *m_series;
    class QDateTimeAxis *m_axisX;
    int m_anzahlTage = 0;
};
