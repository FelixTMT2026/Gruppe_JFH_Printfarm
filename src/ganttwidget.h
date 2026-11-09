#pragma once
// Gantt-Ansicht des Belegungsplans: ein Balken pro geplantem Auftrag,
// gruppiert nach Drucker, Zeit auf der X-Achse (eigene QPainter-Zeichnung).

#include "analyse.h"
#include <QWidget>

class GanttWidget : public QWidget {
    Q_OBJECT
public:
    explicit GanttWidget(QWidget *parent = nullptr);
    void setPlanung(const QList<Planung::Zuweisung> &zuweisungen,
                    const QStringList &druckerIds);
    void leeren();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QList<Planung::Zuweisung> m_zuweisungen;
    QStringList m_druckerIds;
};
