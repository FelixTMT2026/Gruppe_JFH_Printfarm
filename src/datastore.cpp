#include "datastore.h"
#include "csvio.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

DataStore::DataStore(QObject *parent) : QObject(parent) {}

bool DataStore::loadFromFolder(const QString &folder)
{
    m_dataFolder = folder;
    m_drucker.clear(); m_modelle.clear(); m_materialien.clear();
    m_auftraege.clear(); m_wartungen.clear();
    m_modellMap.clear(); m_materialMap.clear(); m_druckerMap.clear();

    bool ok = true;
    ok &= loadModelle(folder + "/modelle.csv");
    ok &= loadMaterialien(folder + "/materialien.csv");
    ok &= loadDrucker(folder + "/drucker.csv");
    ok &= loadAuftraege(folder + "/auftraege.csv");
    loadWartungen(folder + "/wartungen.csv");   // optional für Mindestanforderungen

    for (const auto &m : m_modelle) m_modellMap.insert(m.modellId, &m);
    for (const auto &m : m_materialien) m_materialMap.insert(m.materialId, &m);
    for (const auto &d : m_drucker) m_druckerMap.insert(d.druckerId, &d);
    plausibilitaet();
    emit logChanged();
    return ok;
}

// --- Parser mit Zeilennummern für aussagekräftige Fehlermeldungen -----------
template <typename T, typename F>
static bool parseRows(const QString &path, const char *name,
                      QList<T> &out, QList<CsvError> &errors, F conv)
{
    bool ok = false;
    const auto rows = CsvIo::read(path, &ok, &errors);
    if (!ok) {
        for (auto &e : errors) e.meldung.prepend(QStringLiteral("%1: ").arg(name));
        return false;
    }
    if (rows.isEmpty()) {
        errors.append({0, QStringLiteral("%1: Datei leer").arg(name)});
        return false;
    }
    int zeile = 1; // Kopfzeile
    for (int i = 1; i < rows.size(); ++i) {
        ++zeile;
        T item;
        QString err;
        if (!conv(rows[i], item, err))
            errors.append({zeile, QStringLiteral("%1: %2").arg(name, err)});
        else
            out.append(item);
    }
    return true;
}

static double toDouble(const QString &s, bool *ok = nullptr)
{
    bool good = false;
    const double v = s.trimmed().toDouble(&good);
    if (ok) *ok = good;
    return good ? v : 0.0;
}

bool DataStore::loadModelle(const QString &path)
{
    QList<CsvError> errors;
    bool ok = parseRows(path, "modelle.csv", m_modelle, errors,
        [](const QStringList &r, Modell &m, QString &err) {
            if (r.size() < 11) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            m.modellId = r[0]; m.hersteller = r[1]; m.bezeichnung = r[2]; m.kinematik = r[3];
            m.bauraumXMm = toDouble(r[4]); m.bauraumYMm = toDouble(r[5]); m.bauraumZMm = toDouble(r[6]);
            m.maxDueseTempC = toDouble(r[7]); m.maxBettTempC = toDouble(r[8]);
            m.heizleistungDueseW = toDouble(r[9]); m.heizleistungBettW = toDouble(r[10]);
            if (m.modellId.isEmpty()) { err = QStringLiteral("modell_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        addLog(LogEintrag::Import, QStringLiteral("modelle.csv Zeile %1: %2").arg(e.zeile).arg(e.meldung));
    if (!ok && errors.isEmpty())
        addLog(LogEintrag::Datei, QStringLiteral("modelle.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadMaterialien(const QString &path)
{
    QList<CsvError> errors;
    bool ok = parseRows(path, "materialien.csv", m_materialien, errors,
        [](const QStringList &r, Material &m, QString &err) {
            if (r.size() < 7) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            m.materialId = r[0]; m.bezeichnung = r[1];
            m.dueseTempC = toDouble(r[2]); m.bettTempC = toDouble(r[3]);
            m.dichteGCm3 = toDouble(r[4]); m.preisEurKg = toDouble(r[5]); m.luefterProzent = toDouble(r[6]);
            if (m.materialId.isEmpty()) { err = QStringLiteral("material_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        addLog(LogEintrag::Import, QStringLiteral("materialien.csv Zeile %1: %2").arg(e.zeile).arg(e.meldung));
    if (!ok && errors.isEmpty())
        addLog(LogEintrag::Datei, QStringLiteral("materialien.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadDrucker(const QString &path)
{
    QList<CsvError> errors;
    bool ok = parseRows(path, "drucker.csv", m_drucker, errors,
        [](const QStringList &r, Drucker &d, QString &err) {
            if (r.size() < 11) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            d.druckerId = r[0]; d.modellId = r[1]; d.standort = r[2];
            d.inbetriebnahme = CsvIo::parseDate(r[3]);
            if (d.inbetriebnahme.isValid() == false && !r[3].trimmed().isEmpty())
                err = QStringLiteral("ungültiges Datum inbetriebnahme: %1").arg(r[3]);
            d.betriebsstunden = toDouble(r[4]); d.dueseMm = toDouble(r[5]); d.firmware = r[6];
            d.pidKp = toDouble(r[7]); d.pidKi = toDouble(r[8]); d.pidKd = toDouble(r[9]);
            d.status = r[10];
            if (d.druckerId.isEmpty()) { err = QStringLiteral("drucker_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        addLog(LogEintrag::Import, QStringLiteral("drucker.csv Zeile %1: %2").arg(e.zeile).arg(e.meldung));
    if (!ok && errors.isEmpty())
        addLog(LogEintrag::Datei, QStringLiteral("drucker.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadAuftraege(const QString &path)
{
    QList<CsvError> errors;
    bool ok = parseRows(path, "auftraege.csv", m_auftraege, errors,
        [](const QStringList &r, Auftrag &a, QString &err) {
            if (r.size() < 11) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            a.auftragId = r[0]; a.druckerId = r[1]; a.materialId = r[2]; a.bauteil = r[3];
            a.start = CsvIo::parseDateTime(r[4]); a.ende = CsvIo::parseDateTime(r[5]);
            a.dauerSollMin = toDouble(r[6]); a.filamentSollG = toDouble(r[7]);
            a.ergebnis = r[8]; a.fehlercode = r[9]; a.bemerkung = r[10];
            if (a.auftragId.isEmpty()) { err = QStringLiteral("auftrag_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        addLog(LogEintrag::Import, QStringLiteral("auftraege.csv Zeile %1: %2").arg(e.zeile).arg(e.meldung));
    if (!ok && errors.isEmpty())
        addLog(LogEintrag::Datei, QStringLiteral("auftraege.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadWartungen(const QString &path)
{
    QList<CsvError> errors;
    const bool ok = parseRows(path, "wartungen.csv", m_wartungen, errors,
        [](const QStringList &r, Wartung &w, QString &err) {
            if (r.size() < 6) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            w.wartungId = r[0]; w.druckerId = r[1]; w.datum = CsvIo::parseDate(r[2]);
            w.art = r[3]; w.dauerMin = toDouble(r[4]); w.bemerkung = r[5];
            return !w.wartungId.isEmpty();
        });
    for (const auto &e : errors)
        addLog(LogEintrag::Import, QStringLiteral("wartungen.csv Zeile %1: %2").arg(e.zeile).arg(e.meldung));
    return ok;
}

void DataStore::plausibilitaet()
{
    int fehler = 0;
    for (const auto &d : m_drucker) {
        if (!m_modellMap.contains(d.modellId)) {
            addLog(LogEintrag::Validierung,
                   QStringLiteral("Drucker %1 verweist auf unbekanntes Modell %2").arg(d.druckerId, d.modellId));
            ++fehler;
        }
    }
    for (const auto &a : m_auftraege) {
        if (!m_druckerMap.contains(a.druckerId)) {
            addLog(LogEintrag::Validierung,
                   QStringLiteral("Auftrag %1 verweist auf unbekannten Drucker %2").arg(a.auftragId, a.druckerId), a.auftragId);
            ++fehler;
        }
        if (!m_materialMap.contains(a.materialId)) {
            addLog(LogEintrag::Validierung,
                   QStringLiteral("Auftrag %1 verweist auf unbekanntes Material %2").arg(a.auftragId, a.materialId), a.auftragId);
            ++fehler;
        } else if (const Drucker *d = m_druckerMap.value(a.druckerId, nullptr)) {
            const Modell *m = m_modellMap.value(d->modellId, nullptr);
            const Material *mat = m_materialMap.value(a.materialId);
            if (m && mat) {
                if (mat->dueseTempC > m->maxDueseTempC)
                    addLog(LogEintrag::Validierung,
                           QStringLiteral("Auftrag %1: Materialtemperatur %2 °C > max. Düsentemp %3 °C (Modell %4)")
                               .arg(a.auftragId).arg(mat->dueseTempC).arg(m->maxDueseTempC).arg(m->bezeichnung), a.auftragId);
                if (mat->bettTempC > m->maxBettTempC)
                    addLog(LogEintrag::Validierung,
                           QStringLiteral("Auftrag %1: Betttemperatur %2 °C > max. Betttemp %3 °C (Modell %4)")
                               .arg(a.auftragId).arg(mat->bettTempC).arg(m->maxBettTempC).arg(m->bezeichnung), a.auftragId);
            }
        }
    }
    if (fehler == 0)
        addLog(LogEintrag::System, QStringLiteral("Plausibilitätsprüfung: keine Auffälligkeiten (%1 Drucker, %2 Aufträge)")
                   .arg(m_drucker.size()).arg(m_auftraege.size()));
}

bool DataStore::exportCsv(const QString &folder, QString *errorOut) const
{
    auto writeRows = [&](const QString &name, const QList<QStringList> &rows) {
        return CsvIo::write(folder + "/" + name, rows, errorOut);
    };
    QList<QStringList> druckerRows = {{"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden",
        "duese_mm","firmware","pid_kp","pid_ki","pid_kd","status"}};
    for (const auto &d : m_drucker)
        druckerRows.append({d.druckerId, d.modellId, d.standort, CsvIo::formatDate(d.inbetriebnahme),
            QString::number(d.betriebsstunden), QString::number(d.dueseMm), d.firmware,
            QString::number(d.pidKp), QString::number(d.pidKi), QString::number(d.pidKd), d.status});
    if (!writeRows("drucker_out.csv", druckerRows)) return false;

    QList<QStringList> auftragRows = {{"auftrag_id","drucker_id","material_id","bauteil","start","ende",
        "dauer_soll_min","filament_soll_g","ergebnis","fehlercode","bemerkung"}};
    for (const auto &a : m_auftraege)
        auftragRows.append({a.auftragId, a.druckerId, a.materialId, a.bauteil, CsvIo::formatDateTime(a.start),
            CsvIo::formatDateTime(a.ende), QString::number(a.dauerSollMin), QString::number(a.filamentSollG),
            a.ergebnis, a.fehlercode, a.bemerkung});
    if (!writeRows("auftraege_out.csv", auftragRows)) return false;
    addLog(LogEintrag::System, QStringLiteral("CSV-Export nach %1 geschrieben (drucker_out.csv, auftraege_out.csv)").arg(folder));
    return true;
}

bool DataStore::exportSqlite(const QString &dbPath, QString *errorOut) const
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("export"));
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        if (errorOut) *errorOut = db.lastError().text();
        return false;
    }
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS drucker (drucker_id TEXT PRIMARY KEY, modell_id TEXT, standort TEXT,"
           " inbetriebnahme TEXT, betriebsstunden REAL, duese_mm REAL, firmware TEXT,"
           " pid_kp REAL, pid_ki REAL, pid_kd REAL, status TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS auftraege (auftrag_id TEXT PRIMARY KEY, drucker_id TEXT, material_id TEXT,"
           " bauteil TEXT, start TEXT, ende TEXT, dauer_soll_min REAL, filament_soll_g REAL,"
           " ergebnis TEXT, fehlercode TEXT, bemerkung TEXT)");
    q.exec("DELETE FROM drucker"); q.exec("DELETE FROM auftraege");
    q.prepare("INSERT INTO drucker VALUES (?,?,?,?,?,?,?,?,?,?,?)");
    for (const auto &d : m_drucker) {
        q.addBindValue(d.druckerId); q.addBindValue(d.modellId); q.addBindValue(d.standort);
        q.addBindValue(CsvIo::formatDate(d.inbetriebnahme)); q.addBindValue(d.betriebsstunden);
        q.addBindValue(d.dueseMm); q.addBindValue(d.firmware); q.addBindValue(d.pidKp);
        q.addBindValue(d.pidKi); q.addBindValue(d.pidKd); q.addBindValue(d.status);
        q.exec();
    }
    q.prepare("INSERT INTO auftraege VALUES (?,?,?,?,?,?,?,?,?,?,?)");
    for (const auto &a : m_auftraege) {
        q.addBindValue(a.auftragId); q.addBindValue(a.druckerId); q.addBindValue(a.materialId);
        q.addBindValue(a.bauteil); q.addBindValue(CsvIo::formatDateTime(a.start));
        q.addBindValue(CsvIo::formatDateTime(a.ende)); q.addBindValue(a.dauerSollMin);
        q.addBindValue(a.filamentSollG); q.addBindValue(a.ergebnis); q.addBindValue(a.fehlercode);
        q.addBindValue(a.bemerkung);
        q.exec();
    }
    db.close();
    addLog(LogEintrag::System, QStringLiteral("SQLite-Export nach %1 geschrieben").arg(dbPath));
    return true;
}

bool DataStore::importSqlite(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("import"));
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        addLog(LogEintrag::Datei, QStringLiteral("SQLite-Import fehlgeschlagen: %1").arg(db.lastError().text()));
        return false;
    }
    QSqlQuery q(db);
    m_drucker.clear(); m_auftraege.clear();
    if (q.exec("SELECT * FROM drucker")) {
        while (q.next()) {
            Drucker d;
            d.druckerId = q.value(0).toString(); d.modellId = q.value(1).toString();
            d.standort = q.value(2).toString(); d.inbetriebnahme = CsvIo::parseDate(q.value(3).toString());
            d.betriebsstunden = q.value(4).toDouble(); d.dueseMm = q.value(5).toDouble();
            d.firmware = q.value(6).toString(); d.pidKp = q.value(7).toDouble();
            d.pidKi = q.value(8).toDouble(); d.pidKd = q.value(9).toDouble(); d.status = q.value(10).toString();
            m_drucker.append(d);
        }
    }
    if (q.exec("SELECT * FROM auftraege")) {
        while (q.next()) {
            Auftrag a;
            a.auftragId = q.value(0).toString(); a.druckerId = q.value(1).toString();
            a.materialId = q.value(2).toString(); a.bauteil = q.value(3).toString();
            a.start = CsvIo::parseDateTime(q.value(4).toString()); a.ende = CsvIo::parseDateTime(q.value(5).toString());
            a.dauerSollMin = q.value(6).toDouble(); a.filamentSollG = q.value(7).toDouble();
            a.ergebnis = q.value(8).toString(); a.fehlercode = q.value(9).toString(); a.bemerkung = q.value(10).toString();
            m_auftraege.append(a);
        }
    }
    db.close();
    for (const auto &d : m_drucker) m_druckerMap.insert(d.druckerId, &d);
    addLog(LogEintrag::System, QStringLiteral("SQLite-Import aus %1: %2 Drucker, %3 Aufträge")
               .arg(dbPath).arg(m_drucker.size()).arg(m_auftraege.size()));
    emit logChanged();
    return true;
}

const Modell *DataStore::modell(const QString &id) const { return m_modellMap.value(id, nullptr); }
const Material *DataStore::material(const QString &id) const { return m_materialMap.value(id, nullptr); }
const Drucker *DataStore::drucker(const QString &id) const { return m_druckerMap.value(id, nullptr); }

QList<Auftrag> DataStore::auftraegeOfDrucker(const QString &druckerId) const
{
    QList<Auftrag> out;
    for (const auto &a : m_auftraege)
        if (a.druckerId == druckerId) out.append(a);
    return out;
}

bool DataStore::updateDrucker(const Drucker &d)
{
    for (auto &x : m_drucker) {
        if (x.druckerId == d.druckerId) {
            x = d;
            m_druckerMap.insert(d.druckerId, &x);
            addLog(LogEintrag::System, QStringLiteral("Stammdaten geändert: Drucker %1 (Status=%2, Standort=%3)")
                       .arg(d.druckerId, d.status, d.standort));
            emit logChanged();
            return true;
        }
    }
    return false;
}

void DataStore::addLog(LogEintrag::Kategorie kat, const QString &meldung, const QString &auftragId) const
{
    m_log.append({kat, QDateTime::currentDateTime(), auftragId, meldung});
    const_cast<DataStore*>(this)->logChanged();
}

double DataStore::erfolgsquoteDrucker(const QString &druckerId) const
{
    int fertig = 0, erfolg = 0;
    for (const auto &a : m_auftraege) {
        if (a.druckerId != druckerId) continue;
        if (a.ergebnis == QLatin1String("ausstehend")) continue;
        ++fertig;
        if (a.ergebnis == QLatin1String("erfolgreich")) ++erfolg;
    }
    return fertig == 0 ? -1.0 : double(erfolg) / fertig;
}

double DataStore::filamentverbrauchG(const QString &druckerId) const
{
    double sum = 0;
    for (const auto &a : m_auftraege)
        if (a.druckerId == druckerId && a.ergebnis == QLatin1String("erfolgreich"))
            sum += a.filamentSollG;
    return sum;
}

double DataStore::materialkostenEur(const QString &druckerId) const
{
    double sum = 0;
    for (const auto &a : m_auftraege) {
        if (a.druckerId != druckerId || a.ergebnis != QLatin1String("erfolgreich")) continue;
        if (const Material *m = m_materialMap.value(a.materialId))
            sum += a.filamentSollG / 1000.0 * m->preisEurKg;
    }
    return sum;
}
