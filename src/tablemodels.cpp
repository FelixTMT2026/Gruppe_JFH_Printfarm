// Implementierung siehe tablemodels.h.

#include "tablemodels.h"
#include "csvio.h"
#include <QColor>
#include <QtMath>

PrinterModel::PrinterModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }

void PrinterModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &d : m_store->printer()) {
        if (!m_status.isEmpty() && d.status != m_status) continue;
        if (!m_kinematics.isEmpty()) {
            const DeviceModel *m = m_store->model(d.modelId);
            if (!m || m->kinematics != m_kinematics) continue;
        }
        if (!m_filterText.isEmpty()) {
            const DeviceModel *m = m_store->model(d.modelId);
            const QString hay = QStringLiteral("%1 %2 %3").arg(d.printerId,
                m ? m->description : QString(), d.location).toLower();
            if (!hay.contains(m_filterText.toLower())) continue;
        }
        m_rows.append(&d);
    }
    endResetModel();
}

void PrinterModel::setFilter(const QString &text, const QString &statusFilter, const QString &kinematicsFilter)
{ m_filterText = text; m_status = statusFilter; m_kinematics = kinematicsFilter; rebuild(); }
void PrinterModel::resetFilter() { setFilter({}, {}, {}); }

int PrinterModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int PrinterModel::columnCount(const QModelIndex &) const { return ColCount; }

const Printer *PrinterModel::printerAtRow(int row) const
{ return (row >= 0 && row < m_rows.size()) ? m_rows[row] : nullptr; }

QVariant PrinterModel::data(const QModelIndex &i, int role) const
{
    const Printer *d = printerAtRow(i.row());
    if (!d) return {};
    if (role == Qt::DisplayRole) {
        const DeviceModel *m = m_store->model(d->modelId);
        switch (i.column()) {
        case ColId: return d->printerId;
        case ColModel: return m ? m->description : QStringLiteral("?");
        case ColLocation: return d->location;
        case ColOperatingHours: return qRound64(d->operatingHours);
        case ColStatus: return d->status;
        }
    }
    if (role == Qt::ForegroundRole && i.column() == ColStatus) {

        if (d->status == QLatin1String("defekt") || d->status == QLatin1String("kaputt"))
            return QColor(200, 30, 30);
        if (d->status.isEmpty()) return QColor(110, 110, 110);
        if (d->status == QLatin1String("wartung")) return QColor(200, 130, 0);
        if (d->status == QLatin1String("druckt")) return QColor(0, 90, 200);
        if (d->status == QLatin1String("bereit")) return QColor(0, 140, 60);
    }
    if (role == Qt::ToolTipRole)
        return tr("Doppelklick für Details: %1").arg(d->printerId);
    return {};
}

void PrinterModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();

    std::sort(m_rows.begin(), m_rows.end(), [this, column, order](const Printer *a, const Printer *b) {
        const DeviceModel *ma = m_store->model(a->modelId), *mb = m_store->model(b->modelId);
        int cmp = 0;
        switch (column) {
        case ColId: cmp = a->printerId.compare(b->printerId); break;
        case ColModel: cmp = (ma ? ma->description : QString()).compare(mb ? mb->description : QString()); break;
        case ColLocation: cmp = a->location.compare(b->location); break;
        case ColOperatingHours: cmp = (a->operatingHours < b->operatingHours) ? -1 : (a->operatingHours > b->operatingHours) ? 1 : 0; break;
        case ColStatus: cmp = a->status.compare(b->status); break;
        default: cmp = a->printerId.compare(b->printerId);
        }
        return order == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
    });
    endResetModel();
}

QVariant PrinterModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Drucker-ID");
    case ColModel: return tr("Modell");
    case ColLocation: return tr("Standort");
    case ColOperatingHours: return tr("Betriebsstunden");
    case ColStatus: return tr("Status");
    }
    return {};
}

JobModel::JobModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }

void JobModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &a : m_store->auftraege()) {
        if (!m_printerId.isEmpty() && a.printerId != m_printerId) continue;
        if (!m_resultLabel.isEmpty() && a.jobResult != m_resultLabel) continue;
        if (!m_filterText.isEmpty()) {
            const QString hay = QStringLiteral("%1 %2 %3").arg(a.jobId, a.printerId, a.part).toLower();
            if (!hay.contains(m_filterText.toLower())) continue;
        }
        m_rows.append(&a);
    }
    endResetModel();
}

void JobModel::setFilter(const QString &text, const QString &resultFilter)
{ m_filterText = text; m_resultLabel = resultFilter; rebuild(); }
void JobModel::setPrinterOnly(const QString &printerId) { m_printerId = printerId; rebuild(); }
void JobModel::resetFilter() { setFilter({}, {}); m_printerId.clear(); rebuild(); }

int JobModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int JobModel::columnCount(const QModelIndex &) const { return ColCount; }
const Job *JobModel::jobAtRow(int row) const
{ return (row >= 0 && row < m_rows.size()) ? m_rows[row] : nullptr; }

QVariant JobModel::data(const QModelIndex &i, int role) const
{
    const Job *a = jobAtRow(i.row());
    if (!a) return {};
    if (role == Qt::DisplayRole) {
        switch (i.column()) {
        case ColId: return a->jobId;
        case ColPrinter: return a->printerId;
        case ColPart: return a->part;
        case ColStart: return CsvIo::formatDateTime(a->start);
        case ColDuration: return qRound64(a->plannedMinutes);
        case ColResult: return a->jobResult;
        case ColError: return a->errorCode;
        }
    }
    if (role == Qt::ForegroundRole && i.column() == ColResult) {
        if (a->jobResult == QLatin1String("erfolgreich")) return QColor(0, 140, 60);
        if (a->jobResult == QLatin1String("fehlgeschlagen")) return QColor(200, 30, 30);
        if (a->jobResult == QLatin1String("abgebrochen")) return QColor(200, 130, 0);
        if (a->jobResult == QLatin1String("ausstehend")) return QColor(110, 110, 110);
    }
    if (role == Qt::ToolTipRole)

        return tr("Klick für Verlauf und Auswertung: %1").arg(a->jobId);
    return {};
}

QVariant JobModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Auftrag");
    case ColPrinter: return tr("Drucker");
    case ColPart: return tr("Bauteil");
    case ColStart: return tr("Start");
    case ColDuration: return tr("Soll [min]");
    case ColResult: return tr("Ergebnis");
    case ColError: return tr("Fehlercode");
    }
    return {};
}

void JobModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();

    std::sort(m_rows.begin(), m_rows.end(), [column, order](const Job *a, const Job *b) {
        int cmp = 0;
        switch (column) {
        case ColId: cmp = a->jobId.compare(b->jobId); break;
        case ColPrinter: cmp = a->printerId.compare(b->printerId); break;
        case ColPart: cmp = a->part.compare(b->part); break;
        case ColStart: cmp = (a->start < b->start) ? -1 : (a->start > b->start) ? 1 : 0; break;
        case ColDuration: cmp = (a->plannedMinutes < b->plannedMinutes) ? -1 : (a->plannedMinutes > b->plannedMinutes) ? 1 : 0; break;
        case ColResult: cmp = a->jobResult.compare(b->jobResult); break;
        case ColError: cmp = a->errorCode.compare(b->errorCode); break;
        default: cmp = a->jobId.compare(b->jobId);
        }
        return order == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
    });
    endResetModel();
}

MaterialModel::MaterialModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }

void MaterialModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &m : m_store->materialien())
        m_rows.append(&m);
    endResetModel();
}

int MaterialModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int MaterialModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant MaterialModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (i.row() < 0 || i.row() >= m_rows.size()) return {};
    const Material &m = *m_rows[i.row()];
    switch (i.column()) {
    case ColId: return m.materialId;
    case ColBez: return m.description;
    case ColNozzle: return qRound64(m.nozzleTempC);
    case ColBed: return qRound64(m.bedTempC);
    case ColDensity: return qRound64(m.densityGCm3);
    case ColPrice: return qRound64(m.priceEurKg);
    case ColFan: return qRound64(m.fanPercent);
    case ColWeeklyConsumption:
        return qRound(m_store->meanWeeklyConsumptionKg(m.materialId) * 100.0) / 100.0;
    }
    return {};
}
QVariant MaterialModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Material-ID");
    case ColBez: return tr("Bezeichnung");
    case ColNozzle: return tr("Düse [°C]");
    case ColBed: return tr("Bett [°C]");
    case ColDensity: return tr("Dichte [g/cm³]");
    case ColPrice: return tr("Preis [€/kg]");
    case ColFan: return tr("Lüfter [%]");
    case ColWeeklyConsumption: return tr("Ø Verbrauch/Woche [kg]");
    }
    return {};
}

void MaterialModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();

    QHash<QString, double> weeklyConsumption;
    if (column == ColWeeklyConsumption)
        for (const Material *m : m_rows)
            weeklyConsumption.insert(m->materialId, m_store->meanWeeklyConsumptionKg(m->materialId));

    std::sort(m_rows.begin(), m_rows.end(), [this, column, order, &weeklyConsumption](const Material *a, const Material *b) {
        int cmp = 0;
        switch (column) {
        case ColId: cmp = a->materialId.compare(b->materialId); break;
        case ColBez: cmp = a->description.compare(b->description); break;
        case ColNozzle: cmp = (a->nozzleTempC < b->nozzleTempC) ? -1 : (a->nozzleTempC > b->nozzleTempC) ? 1 : 0; break;
        case ColBed: cmp = (a->bedTempC < b->bedTempC) ? -1 : (a->bedTempC > b->bedTempC) ? 1 : 0; break;
        case ColDensity: cmp = (a->densityGCm3 < b->densityGCm3) ? -1 : (a->densityGCm3 > b->densityGCm3) ? 1 : 0; break;
        case ColPrice: cmp = (a->priceEurKg < b->priceEurKg) ? -1 : (a->priceEurKg > b->priceEurKg) ? 1 : 0; break;
        case ColFan: cmp = (a->fanPercent < b->fanPercent) ? -1 : (a->fanPercent > b->fanPercent) ? 1 : 0; break;
        case ColWeeklyConsumption: {
            const double wa = weeklyConsumption.value(a->materialId);
            const double wb = weeklyConsumption.value(b->materialId);
            cmp = (wa < wb) ? -1 : (wa > wb) ? 1 : 0;
            break;
        }
        default: cmp = a->materialId.compare(b->materialId);
        }
        return order == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
    });
    endResetModel();
}

MaintenanceModel::MaintenanceModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }
void MaintenanceModel::setPrinterOnly(const QString &printerId) { m_printerId = printerId; rebuild(); }
void MaintenanceModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &w : m_store->maintenances())
        if (m_printerId.isEmpty() || w.printerId == m_printerId)
            m_rows.append(&w);
    endResetModel();
}
int MaintenanceModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int MaintenanceModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant MaintenanceModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole || i.row() < 0 || i.row() >= m_rows.size()) return {};
    const Maintenance &w = *m_rows[i.row()];
    switch (i.column()) {
    case ColId: return w.maintenanceId;
    case ColPrinter: return w.printerId;
    case ColDate: return CsvIo::formatDate(w.date);
    case ColArt: return w.kind;
    case ColDuration: return qRound64(w.durationMin);
    case ColNote: return w.note;
    }
    return {};
}
QVariant MaintenanceModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Wartung");
    case ColPrinter: return tr("Drucker");
    case ColDate: return tr("Datum");
    case ColArt: return tr("Art");
    case ColDuration: return tr("Dauer [min]");
    case ColNote: return tr("Bemerkung");
    }
    return {};
}

LogModel::LogModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuildIndex(); }
void LogModel::setCategoryFilter(int category) { m_categoryFilter = category; refresh(); }
void LogModel::refresh()
{
    beginResetModel();
    rebuildIndex();
    endResetModel();
}

void LogModel::rebuildIndex()
{
    m_index.clear();
    const auto &log = m_store->log();
    m_index.reserve(log.size());
    for (int k = log.size() - 1; k >= 0; --k)
        if (m_categoryFilter < 0 || log[k].category == m_categoryFilter)
            m_index.append(k);
}
int LogModel::rowCount(const QModelIndex &) const { return m_index.size(); }
int LogModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant LogModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole) return {};
    if (i.row() < 0 || i.row() >= m_index.size()) return {};
    const auto &log = m_store->log();
    const int k = m_index[i.row()];
    if (k < 0 || k >= log.size()) return {};
    const LogEntry &e = log[k];
    switch (i.column()) {
    case ColTime: return CsvIo::formatDateTime(e.timestamp);
    case ColCategory: {
        switch (e.category) {
        case LogEntry::Import: return tr("Import");
        case LogEntry::Validation: return tr("Validierung");
        case LogEntry::File: return tr("Datei");
        case LogEntry::PrintLog: return tr("Protokoll");
        case LogEntry::System: return tr("System");
        }
        return {};
    }
    case ColJob: return e.jobId;
    case ColMessage: return e.message;
    }
    return {};
}
QVariant LogModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColTime: return tr("Zeitpunkt");
    case ColCategory: return tr("Kategorie");
    case ColJob: return tr("Auftrag");
    case ColMessage: return tr("Meldung");
    }
    return {};
}
