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
    QString aktuellerAuftrag() const { return m_auftragId; }   // zuletzt gezeichneter Auftrag

    // Adaptives Downsampling (LTTB): behält an stark veränderlichen Stellen viele
    // Punkte, reduziert stabile Bereiche auf wenige. Endpunkte bleiben erhalten.
    static QVector<QPointF> lttb(const QVector<QPointF> &points, int threshold);
protected:
    void resizeEvent(QResizeEvent *e) override;   // bei Skalierung neu decimieren
private:
    void render();   // zeichnet Rohdaten (adaptiv oder Standard, je nach Toggle)
    QString m_auftragId;
    QVector<QPointF> m_duesePts, m_bettPts;   // Rohdaten (Zeit in min)
    QVector<QPair<double, QString>> m_events; // (Zeit in min, Name)
    double m_maxTemp = 250.0;
    bool m_rohdaten = false;
    bool m_adaptiv = true;   // Toggle: adaptive (LTTB) vs. Standard (alle Punkte)
    class QCheckBox *m_adaptivBox = nullptr;
};
