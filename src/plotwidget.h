// Temperatur-Plot (Qt Charts): Protokollverlauf und Simulations-Vergleich.
#pragma once

#include <QChartView>
#include <QLineSeries>
#include "datatypes.h"

class PlotWidget : public QChartView {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void showProtocol(const PrintLog &p);

    void showComparison(const QList<QPair<QString, QVector<Sample>>> &curves,
                        bool onlyNozzleActual = true);
    void clear();
    void setDarkMode(bool dark);
    QString currentJob() const { return m_jobId; }

    static QVector<double> adaptiveTimeScale(const QVector<QPointF> &pts, bool adaptive);
protected:
    void resizeEvent(QResizeEvent *e) override;
private:
    void drawChart();
    void adjustAxes();
    QString m_jobId;
    QList<QPair<QString, double>> m_xLabels;
    QVector<QPointF> m_nozzlePts, m_bedPts;
    QVector<QPair<double, QString>> m_events;
    double m_maxTemp = 250.0;
    bool m_rawData = false;
    bool m_adaptive = false;
    class QCheckBox *m_adaptiveBox = nullptr;
};
