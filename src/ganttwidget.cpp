// Implementierung siehe ganttwidget.h.

#include "ganttwidget.h"
#include <QPainter>
#include <QDateTime>

GanttWidget::GanttWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(160);
}

void GanttWidget::clear()
{
    m_assignments.clear();
    m_printerIds.clear();
    update();
}

void GanttWidget::setSchedule(const QList<Planning::Assignment> &assignments,
                             const QStringList &printerIds)
{
    m_assignments = assignments;
    m_printerIds = printerIds;
    update();
}

void GanttWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor textColor = dark ? QColor(230, 236, 243) : QColor(28, 39, 51);
    const QColor gridCol = dark ? QColor(49, 68, 90) : QColor(196, 204, 216);
    const QColor barCol = dark ? QColor(74, 125, 255) : QColor(0, 16, 101);

    p.fillRect(rect(), palette().color(QPalette::Window));
    if (m_assignments.isEmpty()) {
        p.setPen(textColor);
        p.drawText(rect(), Qt::AlignCenter, tr("Noch keine Planung vorhanden - „Auftragsplanung (Greedy)“ klicken."));
        return;
    }
    const int leftMargin = 60, topMargin = 24, rowH = 26;
    QDateTime tMin, tMax;
    for (const auto &z : m_assignments) {
        if (!tMin.isValid() || z.von < tMin) tMin = z.von;
        if (!tMax.isValid() || z.bis > tMax) tMax = z.bis;
    }
    if (!tMin.isValid()) return;
    const double spanSec = qMax(1.0, double(tMin.secsTo(tMax)));
    const int plotWidth = width() - leftMargin - 10;
    const int plotHeight = qMax(40, int(m_printerIds.size()) * rowH);

    p.setPen(textColor);
    p.drawText(QRect(0, 0, leftMargin - 6, topMargin), Qt::AlignRight | Qt::AlignVCenter,
               tr("Drucker"));

    p.setPen(gridCol);
    const int stunden = qMax(1, int(spanSec / 3600) + 1);
    for (int h = 0; h <= stunden; ++h) {
        const double f = double(h) / stunden;
        const int x = leftMargin + int(f * plotWidth);
        p.drawLine(x, topMargin - 6, x, topMargin + plotHeight);
        p.setPen(textColor);
        p.drawText(QRect(x - 30, topMargin + plotHeight + 4, 60, 16), Qt::AlignCenter,
                   tMin.addSecs(qint64(f * spanSec)).toString(QStringLiteral("dd.MM hh:mm")));
        p.setPen(gridCol);
    }

    for (int i = 0; i < m_printerIds.size(); ++i) {
        const int y = topMargin + i * rowH;
        p.setPen(textColor);
        p.drawText(QRect(0, y, leftMargin - 6, rowH), Qt::AlignRight | Qt::AlignVCenter,
                   m_printerIds[i]);
        p.setPen(gridCol);
        p.drawLine(leftMargin, y + rowH - 1, leftMargin + plotWidth, y + rowH - 1);
        for (const auto &z : m_assignments) {
            if (z.printerId != m_printerIds[i]) continue;
            const int x0 = leftMargin + int(double(tMin.secsTo(z.von)) / spanSec * plotWidth);
            const int x1 = leftMargin + int(double(tMin.secsTo(z.bis)) / spanSec * plotWidth);
            p.fillRect(QRect(x0, y + 4, qMax(3, x1 - x0), rowH - 8), barCol);
            p.setPen(dark ? QColor(255, 255, 255) : QColor(255, 255, 255));
            p.drawText(QRect(x0 + 3, y + 4, qMax(3, x1 - x0) - 6, rowH - 8),
                       Qt::AlignLeft | Qt::AlignVCenter, z.jobId);
        }
    }
}
