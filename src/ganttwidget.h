// Gantt-Diagramm (QPainter) fuer den Belegungsplan.
#pragma once

#include "analyse.h"
#include <QWidget>

class GanttWidget : public QWidget {
    Q_OBJECT
public:
    explicit GanttWidget(QWidget *parent = nullptr);
    void setSchedule(const QList<Planning::Assignment> &assignments,
                    const QStringList &printerIds);
    void clear();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QList<Planning::Assignment> m_assignments;
    QStringList m_printerIds;
};
