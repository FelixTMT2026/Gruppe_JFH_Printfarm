#include "tablemodels.h"
#include "csvio.h"

// ---------------------------------------------------------------- DruckerModel
DruckerModel::DruckerModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }

void DruckerModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &d : m_store->drucker()) {
        if (!m_status.isEmpty() && d.status != m_status) continue;
        if (!m_kinematik.isEmpty()) {
            const Modell *m = m_store->modell(d.modellId);
            if (!m || m->kinematik != m_kinematik) continue;
        }
        if (!m_text.isEmpty()) {
            const Modell *m = m_store->modell(d.modellId);
            const QString hay = QStringLiteral("%1 %2 %3").arg(d.druckerId,
                m ? m->bezeichnung : QString(), d.standort).toLower();
            if (!hay.contains(m_text.toLower())) continue;
        }
        m_rows.append(&d);
    }
    endResetModel();
}

void DruckerModel::setFilter(const QString &text, const QString &statusFilter, const QString &kinematikFilter)
{ m_text = text; m_status = statusFilter; m_kinematik = kinematikFilter; rebuild(); }
void DruckerModel::resetFilter() { setFilter({}, {}, {}); }

int DruckerModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int DruckerModel::columnCount(const QModelIndex &) const { return ColCount; }

const Drucker *DruckerModel::druckerAt(int row) const
{ return (row >= 0 && row < m_rows.size()) ? m_rows[row] : nullptr; }

QVariant DruckerModel::data(const QModelIndex &i, int role) const
{
    const Drucker *d = druckerAt(i.row());
    if (!d) return {};
    if (role == Qt::DisplayRole) {
        const Modell *m = m_store->modell(d->modellId);
        switch (i.column()) {
        case ColId: return d->druckerId;
        case ColModell: return m ? m->bezeichnung : QStringLiteral("?");
        case ColStandort: return d->standort;
        case ColBetriebsstunden: return d->betriebsstunden;
        case ColStatus: return d->status;
        }
    }
    if (role == Qt::ToolTipRole)
        return tr("Doppelklick für Details: %1").arg(d->druckerId);
    return {};
}

QVariant DruckerModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Drucker-ID");
    case ColModell: return tr("Modell");
    case ColStandort: return tr("Standort");
    case ColBetriebsstunden: return tr("Betriebsstunden");
    case ColStatus: return tr("Status");
    }
    return {};
}

// ---------------------------------------------------------------- AuftragModel
AuftragModel::AuftragModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }

void AuftragModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &a : m_store->auftraege()) {
        if (!m_druckerId.isEmpty() && a.druckerId != m_druckerId) continue;
        if (!m_ergebnis.isEmpty() && a.ergebnis != m_ergebnis) continue;
        if (!m_text.isEmpty()) {
            const QString hay = QStringLiteral("%1 %2 %3").arg(a.auftragId, a.druckerId, a.bauteil).toLower();
            if (!hay.contains(m_text.toLower())) continue;
        }
        m_rows.append(&a);
    }
    endResetModel();
}

void AuftragModel::setFilter(const QString &text, const QString &ergebnisFilter)
{ m_text = text; m_ergebnis = ergebnisFilter; rebuild(); }
void AuftragModel::setNurDrucker(const QString &druckerId) { m_druckerId = druckerId; rebuild(); }
void AuftragModel::resetFilter() { setFilter({}, {}); m_druckerId.clear(); rebuild(); }

int AuftragModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int AuftragModel::columnCount(const QModelIndex &) const { return ColCount; }
const Auftrag *AuftragModel::auftragAt(int row) const
{ return (row >= 0 && row < m_rows.size()) ? m_rows[row] : nullptr; }

QVariant AuftragModel::data(const QModelIndex &i, int role) const
{
    const Auftrag *a = auftragAt(i.row());
    if (!a) return {};
    if (role == Qt::DisplayRole) {
        switch (i.column()) {
        case ColId: return a->auftragId;
        case ColDrucker: return a->druckerId;
        case ColBauteil: return a->bauteil;
        case ColStart: return CsvIo::formatDateTime(a->start);
        case ColDauer: return a->dauerSollMin;
        case ColErgebnis: return a->ergebnis;
        case ColFehler: return a->fehlercode;
        }
    }
    if (role == Qt::ToolTipRole)
        return tr("Doppelklick für Verlauf und Auswertung: %1").arg(a->auftragId);
    return {};
}

QVariant AuftragModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Auftrag");
    case ColDrucker: return tr("Drucker");
    case ColBauteil: return tr("Bauteil");
    case ColStart: return tr("Start");
    case ColDauer: return tr("Soll [min]");
    case ColErgebnis: return tr("Ergebnis");
    case ColFehler: return tr("Fehlercode");
    }
    return {};
}

// ---------------------------------------------------------------- MaterialModel
MaterialModel::MaterialModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) {}
int MaterialModel::rowCount(const QModelIndex &) const { return m_store->materialien().size(); }
int MaterialModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant MaterialModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole) return {};
    const auto &list = m_store->materialien();
    if (i.row() < 0 || i.row() >= list.size()) return {};
    const Material &m = list[i.row()];
    switch (i.column()) {
    case ColId: return m.materialId;
    case ColBez: return m.bezeichnung;
    case ColDuese: return m.dueseTempC;
    case ColBett: return m.bettTempC;
    case ColDichte: return m.dichteGCm3;
    case ColPreis: return m.preisEurKg;
    case ColLuefter: return m.luefterProzent;
    }
    return {};
}
QVariant MaterialModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Material-ID");
    case ColBez: return tr("Bezeichnung");
    case ColDuese: return tr("Düse [°C]");
    case ColBett: return tr("Bett [°C]");
    case ColDichte: return tr("Dichte [g/cm³]");
    case ColPreis: return tr("Preis [€/kg]");
    case ColLuefter: return tr("Lüfter [%]");
    }
    return {};
}

// ---------------------------------------------------------------- ModellModel
ModellModel::ModellModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) {}
int ModellModel::rowCount(const QModelIndex &) const { return m_store->modelle().size(); }
int ModellModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant ModellModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole) return {};
    const auto &list = m_store->modelle();
    if (i.row() < 0 || i.row() >= list.size()) return {};
    const Modell &m = list[i.row()];
    switch (i.column()) {
    case ColId: return m.modellId;
    case ColHersteller: return m.hersteller;
    case ColBez: return m.bezeichnung;
    case ColKinematik: return m.kinematik;
    case ColBauraum: return QStringLiteral("%1×%2×%3").arg(m.bauraumXMm).arg(m.bauraumYMm).arg(m.bauraumZMm);
    case ColMaxDuese: return m.maxDueseTempC;
    case ColMaxBett: return m.maxBettTempC;
    }
    return {};
}
QVariant ModellModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Modell-ID");
    case ColHersteller: return tr("Hersteller");
    case ColBez: return tr("Bezeichnung");
    case ColKinematik: return tr("Kinematik");
    case ColBauraum: return tr("Bauraum [mm]");
    case ColMaxDuese: return tr("max. Düse [°C]");
    case ColMaxBett: return tr("max. Bett [°C]");
    }
    return {};
}

// ---------------------------------------------------------------- WartungModel
WartungModel::WartungModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) { rebuild(); }
void WartungModel::setNurDrucker(const QString &druckerId) { m_druckerId = druckerId; rebuild(); }
void WartungModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    for (const auto &w : m_store->wartungen())
        if (m_druckerId.isEmpty() || w.druckerId == m_druckerId)
            m_rows.append(&w);
    endResetModel();
}
int WartungModel::rowCount(const QModelIndex &) const { return m_rows.size(); }
int WartungModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant WartungModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole || i.row() < 0 || i.row() >= m_rows.size()) return {};
    const Wartung &w = *m_rows[i.row()];
    switch (i.column()) {
    case ColId: return w.wartungId;
    case ColDrucker: return w.druckerId;
    case ColDatum: return CsvIo::formatDate(w.datum);
    case ColArt: return w.art;
    case ColDauer: return w.dauerMin;
    case ColBemerkung: return w.bemerkung;
    }
    return {};
}
QVariant WartungModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColId: return tr("Wartung");
    case ColDrucker: return tr("Drucker");
    case ColDatum: return tr("Datum");
    case ColArt: return tr("Art");
    case ColDauer: return tr("Dauer [min]");
    case ColBemerkung: return tr("Bemerkung");
    }
    return {};
}

// ---------------------------------------------------------------- LogModel
LogModel::LogModel(const DataStore *store, QObject *parent)
    : QAbstractTableModel(parent), m_store(store) {}
void LogModel::setKategorieFilter(int kat) { m_katFilter = kat; refresh(); }
void LogModel::refresh() { beginResetModel(); endResetModel(); }
int LogModel::rowCount(const QModelIndex &) const
{
    if (m_katFilter < 0) return m_store->log().size();
    int n = 0;
    for (const auto &e : m_store->log())
        if (e.kategorie == m_katFilter) ++n;
    return n;
}
int LogModel::columnCount(const QModelIndex &) const { return ColCount; }
QVariant LogModel::data(const QModelIndex &i, int role) const
{
    if (role != Qt::DisplayRole) return {};
    int row = i.row();
    const auto &log = m_store->log();
    int idx = -1;
    for (int k = log.size() - 1; k >= 0; --k) {   // neueste zuerst
        if (m_katFilter < 0 || log[k].kategorie == m_katFilter) {
            ++idx;
            if (idx == row) {
                const LogEintrag &e = log[k];
                switch (i.column()) {
                case ColZeit: return CsvIo::formatDateTime(e.zeitpunkt);
                case ColKategorie: {
                    switch (e.kategorie) {
                    case LogEintrag::Import: return tr("Import");
                    case LogEintrag::Validierung: return tr("Validierung");
                    case LogEintrag::Datei: return tr("Datei");
                    case LogEintrag::Protokoll: return tr("Protokoll");
                    case LogEintrag::System: return tr("System");
                    }
                    return {};
                }
                case ColAuftrag: return e.auftragId;
                case ColMeldung: return e.meldung;
                }
                return {};
            }
        }
    }
    return {};
}
QVariant LogModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (s) {
    case ColZeit: return tr("Zeitpunkt");
    case ColKategorie: return tr("Kategorie");
    case ColAuftrag: return tr("Auftrag");
    case ColMeldung: return tr("Meldung");
    }
    return {};
}
