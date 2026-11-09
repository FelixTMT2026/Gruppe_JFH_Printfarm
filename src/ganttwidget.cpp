#include "ganttwidget.h"
#include <QPainter>
#include <QDateTime>

GanttWidget::GanttWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(160);
}

void GanttWidget::leeren()
{
    m_zuweisungen.clear();
    m_druckerIds.clear();
    update();
}

void GanttWidget::setPlanung(const QList<Planung::Zuweisung> &zuweisungen,
                             const QStringList &druckerIds)
{
    m_zuweisungen = zuweisungen;
    m_druckerIds = druckerIds;
    update();
}

void GanttWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor textCol = dark ? QColor(230, 236, 243) : QColor(28, 39, 51);
    const QColor gridCol = dark ? QColor(49, 68, 90) : QColor(196, 204, 216);
    const QColor barCol = dark ? QColor(74, 125, 255) : QColor(0, 16, 101);

    p.fillRect(rect(), palette().color(QPalette::Window));
    if (m_zuweisungen.isEmpty()) {
        p.setPen(textCol);
        p.drawText(rect(), Qt::AlignCenter, tr("Noch keine Planung vorhanden — „Auftragsplanung (Greedy)“ klicken."));
        return;
    }
    const int leftMargin = 60, topMargin = 24, rowH = 26;
    QDateTime tMin, tMax;
    for (const auto &z : m_zuweisungen) {
        if (!tMin.isValid() || z.von < tMin) tMin = z.von;
        if (!tMax.isValid() || z.bis > tMax) tMax = z.bis;
    }
    if (!tMin.isValid()) return;
    const double spanSec = qMax(1.0, double(tMin.secsTo(tMax)));
    const int plotW = width() - leftMargin - 10;
    const int plotH = qMax(40, int(m_druckerIds.size()) * rowH);

    p.setPen(textCol);
    p.drawText(QRect(0, 0, leftMargin - 6, topMargin), Qt::AlignRight | Qt::AlignVCenter,
               tr("Drucker"));
    // X-Achse: Stunden-Markierungen
    p.setPen(gridCol);
    const int stunden = qMax(1, int(spanSec / 3600) + 1);
    for (int h = 0; h <= stunden; ++h) {
        const double f = double(h) / stunden;
        const int x = leftMargin + int(f * plotW);
        p.drawLine(x, topMargin - 6, x, topMargin + plotH);
        p.setPen(textCol);
        p.drawText(QRect(x - 30, topMargin + plotH + 4, 60, 16), Qt::AlignCenter,
                   tMin.addSecs(qint64(f * spanSec)).toString(QStringLiteral("dd.MM hh:mm")));
        p.setPen(gridCol);
    }
    // Zeilen + Balken
    for (int i = 0; i < m_druckerIds.size(); ++i) {
        const int y = topMargin + i * rowH;
        p.setPen(textCol);
        p.drawText(QRect(0, y, leftMargin - 6, rowH), Qt::AlignRight | Qt::AlignVCenter,
                   m_druckerIds[i]);
        p.setPen(gridCol);
        p.drawLine(leftMargin, y + rowH - 1, leftMargin + plotW, y + rowH - 1);
        for (const auto &z : m_zuweisungen) {
            if (z.druckerId != m_druckerIds[i]) continue;
            const int x0 = leftMargin + int(double(tMin.secsTo(z.von)) / spanSec * plotW);
            const int x1 = leftMargin + int(double(tMin.secsTo(z.bis)) / spanSec * plotW);
            p.fillRect(QRect(x0, y + 4, qMax(3, x1 - x0), rowH - 8), barCol);
            p.setPen(dark ? QColor(255, 255, 255) : QColor(255, 255, 255));
            p.drawText(QRect(x0 + 3, y + 4, qMax(3, x1 - x0) - 6, rowH - 8),
                       Qt::AlignLeft | Qt::AlignVCenter, z.auftragId);
        }
    }
}
