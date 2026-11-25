// Implementierung siehe datastore.h.

#include "datastore.h"
#include "csvio.h"
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

DataStore::DataStore(QObject *parent) : QObject(parent) {}

bool DataStore::loadFromFolder(const QString &folder)
{
    m_dataFolder = folder;
    m_printers.clear(); m_models.clear(); m_materialien.clear();
    m_auftraege.clear(); m_maintenances.clear();
    m_modelMap.clear(); m_materialMap.clear(); m_printerMap.clear();

    bool ok = true;
    ok &= loadModels(folder + "/modelle.csv");
    ok &= loadMaterials(folder + "/materialien.csv");
    ok &= loadPrinters(folder + "/drucker.csv");
    ok &= loadJobs(folder + "/auftraege.csv");
    loadMaintenances(folder + "/wartungen.csv");

    for (const auto &m : m_models) m_modelMap.insert(m.modelId, &m);
    for (const auto &m : m_materialien) m_materialMap.insert(m.materialId, &m);
    for (const auto &d : m_printers) m_printerMap.insert(d.printerId, &d);
    validateConsistency();
    emit logChanged();
    emit dataLoaded();
    return ok;
}

template <typename T, typename F>
static bool parseRows(const QString &path, const char *name,
                      QList<T> &out, QList<CsvError> &errors, F conv)
{
    bool ok = false;
    const auto rows = CsvIo::readCsv(path, &ok, &errors);
    if (!ok) {
        for (auto &e : errors) e.message.prepend(QStringLiteral("%1: ").arg(name));
        return false;
    }
    if (rows.isEmpty()) {
        errors.append({0, QStringLiteral("%1: Datei leer").arg(name)});
        return false;
    }
    int line = 1;
    for (int i = 1; i < rows.size(); ++i) {
        ++line;
        T item;
        QString err;
        if (!conv(rows[i], line, item, err))
            errors.append({line, QStringLiteral("%1: %2").arg(name, err)});
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

// Defensives Zahlen-Parsen: ungueltige Werte landen als Log-Eintrag, nicht als stille 0.
static double numberOrError(const QString &raw, const QString &field, int line,
                             const char *datei, QList<CsvError> &warns, bool *ok = nullptr)
{
    bool good = false;
    const double v = toDouble(raw, &good);
    if (ok) *ok = good;
    if (!good && !raw.trimmed().isEmpty())
        warns.append({line, QStringLiteral("%1 Zeile %2: „%3“ ist keine Zahl (Feld %4) -> 0")
                          .arg(datei).arg(line).arg(raw, field)});
    return v;
}

bool DataStore::loadModels(const QString &path)
{
    QList<CsvError> errors;
    QList<CsvError> warns;
    bool ok = parseRows(path, "modelle.csv", m_models, errors,
        [&](const QStringList &r, int line, DeviceModel &m, QString &err) {
            if (r.size() < 11) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            m.modelId = r[0]; m.manufacturer = r[1]; m.description = r[2]; m.kinematics = r[3];
            m.buildXMm = numberOrError(r[4], QStringLiteral("bauraum_x_mm"), line, "modelle.csv", warns);
            m.buildYMm = numberOrError(r[5], QStringLiteral("bauraum_y_mm"), line, "modelle.csv", warns);
            m.buildZMm = numberOrError(r[6], QStringLiteral("bauraum_z_mm"), line, "modelle.csv", warns);
            m.maxNozzleTempC = numberOrError(r[7], QStringLiteral("max_duesentemp_c"), line, "modelle.csv", warns);
            m.maxBedTempC = numberOrError(r[8], QStringLiteral("max_betttemp_c"), line, "modelle.csv", warns);
            m.nozzleHeaterW = numberOrError(r[9], QStringLiteral("heizleistung_duese_w"), line, "modelle.csv", warns);
            m.bedHeaterW = numberOrError(r[10], QStringLiteral("heizleistung_bett_w"), line, "modelle.csv", warns);
            if (m.modelId.isEmpty()) { err = QStringLiteral("modell_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        log(ok ? LogEntry::Import : LogEntry::File, QStringLiteral("modelle.csv Zeile %1: %2").arg(e.line).arg(e.message));
    for (const auto &w : warns)
        log(LogEntry::Validation, w.message);
    if (!ok && errors.isEmpty())
        log(LogEntry::File, QStringLiteral("modelle.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadMaterials(const QString &path)
{
    QList<CsvError> errors;
    QList<CsvError> warns;
    bool ok = parseRows(path, "materialien.csv", m_materialien, errors,
        [&](const QStringList &r, int line, Material &m, QString &err) {
            if (r.size() < 7) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            m.materialId = r[0]; m.description = r[1];
            m.nozzleTempC = numberOrError(r[2], QStringLiteral("duesentemp_c"), line, "materialien.csv", warns);
            m.bedTempC = numberOrError(r[3], QStringLiteral("betttemp_c"), line, "materialien.csv", warns);
            m.densityGCm3 = numberOrError(r[4], QStringLiteral("dichte_g_cm3"), line, "materialien.csv", warns);
            m.priceEurKg = numberOrError(r[5], QStringLiteral("preis_eur_kg"), line, "materialien.csv", warns);
            m.fanPercent = numberOrError(r[6], QStringLiteral("luefter_prozent"), line, "materialien.csv", warns);
            if (m.materialId.isEmpty()) { err = QStringLiteral("material_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        log(ok ? LogEntry::Import : LogEntry::File, QStringLiteral("materialien.csv Zeile %1: %2").arg(e.line).arg(e.message));
    for (const auto &w : warns)
        log(LogEntry::Validation, w.message);
    if (!ok && errors.isEmpty())
        log(LogEntry::File, QStringLiteral("materialien.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadPrinters(const QString &path)
{
    QList<CsvError> errors;
    QList<CsvError> warns;
    bool ok = parseRows(path, "drucker.csv", m_printers, errors,
        [&](const QStringList &r, int line, Printer &d, QString &err) {
            if (r.size() < 11) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            d.printerId = r[0]; d.modelId = r[1]; d.location = r[2];
            d.commissionedOn = CsvIo::parseDate(r[3]);

            if (d.commissionedOn.isValid() == false && !r[3].trimmed().isEmpty())
                warns.append({line, QStringLiteral("drucker.csv Zeile %1: ungültiges Datum inbetriebnahme: %2").arg(line).arg(r[3])});
            d.operatingHours = numberOrError(r[4], QStringLiteral("betriebsstunden"), line, "drucker.csv", warns);
            d.nozzleMm = numberOrError(r[5], QStringLiteral("duese_mm"), line, "drucker.csv", warns);
            d.firmware = r[6];
            d.pidKp = numberOrError(r[7], QStringLiteral("pid_kp"), line, "drucker.csv", warns);
            d.pidKi = numberOrError(r[8], QStringLiteral("pid_ki"), line, "drucker.csv", warns);
            d.pidKd = numberOrError(r[9], QStringLiteral("pid_kd"), line, "drucker.csv", warns);
            d.status = r[10];
            if (d.printerId.isEmpty()) { err = QStringLiteral("drucker_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        log(ok ? LogEntry::Import : LogEntry::File, QStringLiteral("drucker.csv Zeile %1: %2").arg(e.line).arg(e.message));
    for (const auto &w : warns)
        log(LogEntry::Validation, w.message);
    if (!ok && errors.isEmpty())
        log(LogEntry::File, QStringLiteral("drucker.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadJobs(const QString &path)
{
    QList<CsvError> errors;
    QList<CsvError> warns;
    bool ok = parseRows(path, "auftraege.csv", m_auftraege, errors,
        [&](const QStringList &r, int line, Job &a, QString &err) {
            if (r.size() < 11) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            a.jobId = r[0]; a.printerId = r[1]; a.materialId = r[2]; a.part = r[3];
            a.start = CsvIo::parseDateTime(r[4]); a.ende = CsvIo::parseDateTime(r[5]);
            a.plannedMinutes = numberOrError(r[6], QStringLiteral("dauer_soll_min"), line, "auftraege.csv", warns);
            a.plannedFilamentG = numberOrError(r[7], QStringLiteral("filament_soll_g"), line, "auftraege.csv", warns);
            a.jobResult = r[8]; a.errorCode = r[9]; a.note = r[10];
            if (a.jobId.isEmpty()) { err = QStringLiteral("auftrag_id leer"); return false; }
            return true;
        });
    for (const auto &e : errors)
        log(ok ? LogEntry::Import : LogEntry::File, QStringLiteral("auftraege.csv Zeile %1: %2").arg(e.line).arg(e.message));
    for (const auto &w : warns)
        log(LogEntry::Validation, w.message);
    if (!ok && errors.isEmpty())
        log(LogEntry::File, QStringLiteral("auftraege.csv konnte nicht geladen werden"));
    return ok;
}

bool DataStore::loadMaintenances(const QString &path)
{
    QList<CsvError> errors;
    QList<CsvError> warns;
    const bool ok = parseRows(path, "wartungen.csv", m_maintenances, errors,
        [&](const QStringList &r, int line, Maintenance &w, QString &err) {
            if (r.size() < 6) { err = QStringLiteral("zu wenige Spalten (%1)").arg(r.size()); return false; }
            w.maintenanceId = r[0]; w.printerId = r[1]; w.date = CsvIo::parseDate(r[2]);
            if (w.date.isValid() == false && !r[2].trimmed().isEmpty())
                warns.append({line, QStringLiteral("wartungen.csv Zeile %1: ungültiges Datum: %2").arg(line).arg(r[2])});
            w.kind = r[3];
            w.durationMin = numberOrError(r[4], QStringLiteral("dauer_min"), line, "wartungen.csv", warns);
            w.note = r[5];
            return !w.maintenanceId.isEmpty();
        });
    for (const auto &e : errors)
        log(ok ? LogEntry::Import : LogEntry::File, QStringLiteral("wartungen.csv Zeile %1: %2").arg(e.line).arg(e.message));
    for (const auto &w : warns)
        log(LogEntry::Validation, w.message);
    return ok;
}

static constexpr double kMaxNozzleTempPlausibleC = 1000.0;
static constexpr double kMaxBedTempPlausibleC = 300.0;
static constexpr double kNozzleMmMin = 0.1;
static constexpr double kNozzleMmMax = 2.0;

// Plausibilitaet: Duplikat-IDs, Wertebereiche, Referenzen, Material vs. Modell.
void DataStore::validateConsistency()
{
    int error = 0;
    checkDuplicateIds();

    for (const auto &m : m_models) {
        if (m.maxNozzleTempC > kMaxNozzleTempPlausibleC)
            log(LogEntry::Validation,
                   QStringLiteral("Modell %1: max. Düsentemp %2 °C unrealistisch (> %3 °C)")
                       .arg(m.modelId).arg(m.maxNozzleTempC).arg(kMaxNozzleTempPlausibleC));
        if (m.maxBedTempC > kMaxBedTempPlausibleC)
            log(LogEntry::Validation,
                   QStringLiteral("Modell %1: max. Betttemp %2 °C unrealistisch (> %3 °C)")
                       .arg(m.modelId).arg(m.maxBedTempC).arg(kMaxBedTempPlausibleC));
    }
    validatePrinters(error);
    validateJobs(error);
    if (error == 0)
        log(LogEntry::System, QStringLiteral("Plausibilitätsprüfung: keine Auffälligkeiten (%1 Drucker, %2 Aufträge)")
                   .arg(m_printers.size()).arg(m_auftraege.size()));
}

void DataStore::checkDuplicateIds()
{
    auto check = [this](const QString &datei, const QList<QString> &ids) {
        QSet<QString> seen;
        for (const auto &id : ids) {
            if (seen.contains(id))
                log(LogEntry::Validation,
                       QStringLiteral("%1: Duplikat-ID „%2“ mehrfach vorhanden").arg(datei, id));
            seen.insert(id);
        }
    };
    QList<QString> ids;
    for (const auto &x : m_models) ids.append(x.modelId);
    check(QStringLiteral("modelle.csv"), ids);
    ids.clear();
    for (const auto &x : m_materialien) ids.append(x.materialId);
    check(QStringLiteral("materialien.csv"), ids);
    ids.clear();
    for (const auto &x : m_printers) ids.append(x.printerId);
    check(QStringLiteral("drucker.csv"), ids);
    ids.clear();
    for (const auto &x : m_auftraege) ids.append(x.jobId);
    check(QStringLiteral("auftraege.csv"), ids);
    ids.clear();
    for (const auto &x : m_maintenances) ids.append(x.maintenanceId);
    check(QStringLiteral("wartungen.csv"), ids);
}

void DataStore::validatePrinters(int &error)
{
    const QSet<QString> allowedStatus = {QStringLiteral("bereit"), QStringLiteral("druckt"),
                                          QStringLiteral("wartung"), QStringLiteral("defekt")};
    for (const auto &d : m_printers) {
        if (!allowedStatus.contains(d.status))
            log(LogEntry::Validation,
                   QStringLiteral("Drucker %1 hat ungültigen Status „%2“ (erlaubt: bereit/druckt/wartung/defekt)")
                       .arg(d.printerId, d.status.isEmpty() ? QStringLiteral("(leer)") : d.status));
        if (!m_modelMap.contains(d.modelId)) {
            log(LogEntry::Validation,
                   QStringLiteral("Drucker %1 verweist auf unbekanntes Modell %2").arg(d.printerId, d.modelId));
            ++error;
        }
        if (d.operatingHours < 0)
            log(LogEntry::Validation,
                   QStringLiteral("Drucker %1: negative Betriebsstunden (%2)").arg(d.printerId).arg(d.operatingHours));
        if (d.nozzleMm < kNozzleMmMin || d.nozzleMm > kNozzleMmMax)
            log(LogEntry::Validation,
                   QStringLiteral("Drucker %1: Düse %2 mm außerhalb des plausiblen Bereichs %3–%4 mm")
                       .arg(d.printerId).arg(d.nozzleMm).arg(kNozzleMmMin).arg(kNozzleMmMax));
        if (d.pidKp < 0 || d.pidKi < 0 || d.pidKd < 0)
            log(LogEntry::Validation,
                   QStringLiteral("Drucker %1: negativer PID-Wert (Kp=%2 Ki=%3 Kd=%4) – Regler würde destabilisieren")
                       .arg(d.printerId).arg(d.pidKp).arg(d.pidKi).arg(d.pidKd));
    }
}

void DataStore::validateJobs(int &error)
{
    const QSet<QString> allowedResults = {QStringLiteral("erfolgreich"), QStringLiteral("abgebrochen"),
                                            QStringLiteral("fehlgeschlagen"), QStringLiteral("ausstehend")};
    for (const auto &a : m_auftraege) {
        if (!allowedResults.contains(a.jobResult))
            log(LogEntry::Validation,
                   QStringLiteral("Auftrag %1 hat ungültiges Ergebnis „%2“ (erlaubt: erfolgreich/abgebrochen/fehlgeschlagen/ausstehend)")
                       .arg(a.jobId, a.jobResult), a.jobId);
        if (!m_printerMap.contains(a.printerId)) {
            log(LogEntry::Validation,
                   QStringLiteral("Auftrag %1 verweist auf unbekannten Drucker %2").arg(a.jobId, a.printerId), a.jobId);
            ++error;
        }
        if (!m_materialMap.contains(a.materialId)) {
            log(LogEntry::Validation,
                   QStringLiteral("Auftrag %1 verweist auf unbekanntes Material %2").arg(a.jobId, a.materialId), a.jobId);
            ++error;
        } else if (const Printer *d = m_printerMap.value(a.printerId, nullptr)) {
            const DeviceModel *m = m_modelMap.value(d->modelId, nullptr);
            const Material *mat = m_materialMap.value(a.materialId);
            if (m && mat) {
                if (mat->nozzleTempC > m->maxNozzleTempC)
                    log(LogEntry::Validation,
                           QStringLiteral("Auftrag %1: Materialtemperatur %2 °C > max. Düsentemp %3 °C (Modell %4)")
                               .arg(a.jobId).arg(mat->nozzleTempC).arg(m->maxNozzleTempC).arg(m->description), a.jobId);
                if (mat->bedTempC > m->maxBedTempC)
                    log(LogEntry::Validation,
                           QStringLiteral("Auftrag %1: Betttemperatur %2 °C > max. Betttemp %3 °C (Modell %4)")
                               .arg(a.jobId).arg(mat->bedTempC).arg(m->maxBedTempC).arg(m->description), a.jobId);
            }
        }
    }
}

bool DataStore::exportCsv(const QString &folder, QString *errorOut) const
{
    auto writeRows = [&](const QString &name, const QList<QStringList> &rows) {
        return CsvIo::writeCsv(folder + "/" + name, rows, errorOut);
    };
    QList<QStringList> printerRows = {{"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden",
        "duese_mm","firmware","pid_kp","pid_ki","pid_kd","status"}};
    for (const auto &d : m_printers)
        printerRows.append({d.printerId, d.modelId, d.location, CsvIo::formatDate(d.commissionedOn),
            QString::number(d.operatingHours), QString::number(d.nozzleMm), d.firmware,
            QString::number(d.pidKp), QString::number(d.pidKi), QString::number(d.pidKd), d.status});
    if (!writeRows("drucker_out.csv", printerRows)) return false;

    QList<QStringList> jobRows = {{"auftrag_id","drucker_id","material_id","bauteil","start","ende",
        "dauer_soll_min","filament_soll_g","ergebnis","fehlercode","bemerkung"}};
    for (const auto &a : m_auftraege)
        jobRows.append({a.jobId, a.printerId, a.materialId, a.part, CsvIo::formatDateTime(a.start),
            CsvIo::formatDateTime(a.ende), QString::number(a.plannedMinutes), QString::number(a.plannedFilamentG),
            a.jobResult, a.errorCode, a.note});
    if (!writeRows("auftraege_out.csv", jobRows)) return false;
    log(LogEntry::System, QStringLiteral("CSV-Export nach %1 geschrieben (drucker_out.csv, auftraege_out.csv)").arg(folder));
    return true;
}

bool DataStore::exportSqlite(const QString &dbPath, QString *errorOut) const
{

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("export"));
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        if (errorOut) *errorOut = db.lastError().text();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("export"));
        return false;
    }
    QSqlQuery q(db);

    int error = 0;
    auto execChecked = [&](const QString &sql) {
        if (!q.exec(sql)) {
            ++error;
            log(LogEntry::File, QStringLiteral("SQLite-Export: SQL fehlgeschlagen: %1 (%2)")
                       .arg(sql, q.lastError().text()));
        }
    };
    QSet<QString> seen;
    auto insertRow = [&](const QString &table, const QString &id) {
        if (seen.contains(id))
            log(LogEntry::Validation, QStringLiteral("SQLite-Export: Duplikat-ID „%1“ in %2 – letzter Eintrag gewinnt (INSERT OR REPLACE, konsistent zur Map)")
                       .arg(id, table));
        seen.insert(id);
        if (!q.exec()) {
            ++error;
            log(LogEntry::File, QStringLiteral("SQLite-Export: Datensatz %1 „%2“ wurde verworfen: %3")
                       .arg(table, id, q.lastError().text()));
        }
    };

    const QString ddlPrinters = QStringLiteral("CREATE TABLE IF NOT EXISTS drucker (drucker_id TEXT PRIMARY KEY, modell_id TEXT, standort TEXT,"
                 " inbetriebnahme TEXT, betriebsstunden REAL, duese_mm REAL, firmware TEXT,"
                 " pid_kp REAL, pid_ki REAL, pid_kd REAL, status TEXT)");
    const QString ddlJobs = QStringLiteral("CREATE TABLE IF NOT EXISTS auftraege (auftrag_id TEXT PRIMARY KEY, drucker_id TEXT, material_id TEXT,"
                 " bauteil TEXT, start TEXT, ende TEXT, dauer_soll_min REAL, filament_soll_g REAL,"
                 " ergebnis TEXT, fehlercode TEXT, bemerkung TEXT)");

    const QString ddlModels = QStringLiteral("CREATE TABLE IF NOT EXISTS modelle (modell_id TEXT PRIMARY KEY, hersteller TEXT, bezeichnung TEXT,"
                 " kinematik TEXT, bauraum_x_mm REAL, bauraum_y_mm REAL, bauraum_z_mm REAL,"
                 " max_duesentemp_c REAL, max_betttemp_c REAL, heizleistung_duese_w REAL, heizleistung_bett_w REAL)");
    const QString ddlMaterials = QStringLiteral("CREATE TABLE IF NOT EXISTS materialien (material_id TEXT PRIMARY KEY, bezeichnung TEXT, duese_temp_c REAL,"
                 " bett_temp_c REAL, dichte_g_cm3 REAL, preis_eur_kg REAL, luefter_prozent REAL)");
    const QString ddlMaintenances = QStringLiteral("CREATE TABLE IF NOT EXISTS wartungen (wartung_id TEXT PRIMARY KEY, drucker_id TEXT, datum TEXT,"
                 " art TEXT, dauer_min REAL, bemerkung TEXT)");

    auto writeTable = [&](const QString &name, const QString &ddl, const QList<QPair<QString, QVariantList>> &lines) {
        execChecked(ddl);
        execChecked(QStringLiteral("DELETE FROM %1").arg(name));
        q.prepare(QStringLiteral("INSERT OR REPLACE INTO %1 VALUES (%2)")
                      .arg(name, QStringLiteral("?,").repeated(lines.isEmpty() ? 1 : lines.first().second.size()).chopped(1)));
        seen.clear();
        for (const auto &line : lines) {
            for (const QVariant &v : line.second)
                q.addBindValue(v);
            insertRow(name, line.first);
        }
    };

    execChecked("BEGIN");
    writeTable(QStringLiteral("drucker"), ddlPrinters, [&]{
        QList<QPair<QString, QVariantList>> rows;
        for (const auto &d : m_printers)
            rows.append({d.printerId, {d.printerId, d.modelId, d.location, CsvIo::formatDate(d.commissionedOn),
                d.operatingHours, d.nozzleMm, d.firmware, d.pidKp, d.pidKi, d.pidKd, d.status}});
        return rows; }());
    writeTable(QStringLiteral("auftraege"), ddlJobs, [&]{
        QList<QPair<QString, QVariantList>> rows;
        for (const auto &a : m_auftraege)
            rows.append({a.jobId, {a.jobId, a.printerId, a.materialId, a.part,
                CsvIo::formatDateTime(a.start), CsvIo::formatDateTime(a.ende),
                a.plannedMinutes, a.plannedFilamentG, a.jobResult, a.errorCode, a.note}});
        return rows; }());
    writeTable(QStringLiteral("modelle"), ddlModels, [&]{
        QList<QPair<QString, QVariantList>> rows;
        for (const auto &m : m_models)
            rows.append({m.modelId, {m.modelId, m.manufacturer, m.description, m.kinematics, m.buildXMm,
                m.buildYMm, m.buildZMm, m.maxNozzleTempC, m.maxBedTempC, m.nozzleHeaterW, m.bedHeaterW}});
        return rows; }());
    writeTable(QStringLiteral("materialien"), ddlMaterials, [&]{
        QList<QPair<QString, QVariantList>> rows;
        for (const auto &m : m_materialien)
            rows.append({m.materialId, {m.materialId, m.description, m.nozzleTempC, m.bedTempC,
                m.densityGCm3, m.priceEurKg, m.fanPercent}});
        return rows; }());
    writeTable(QStringLiteral("wartungen"), ddlMaintenances, [&]{
        QList<QPair<QString, QVariantList>> rows;
        for (const auto &w : m_maintenances)
            rows.append({w.maintenanceId, {w.maintenanceId, w.printerId, CsvIo::formatDate(w.date),
                w.kind, w.durationMin, w.note}});
        return rows; }());
    execChecked("COMMIT");
    q = QSqlQuery();
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(QStringLiteral("export"));
    if (error > 0) {
        if (errorOut) *errorOut = QStringLiteral("%1 SQL-Anweisung(en) fehlgeschlagen – betroffene Datensaetze siehe Log").arg(error);
        log(LogEntry::File, QStringLiteral("SQLite-Export nach %1 unvollstaendig: %2 Anweisung(en) fehlgeschlagen (verworfene Datensaetze siehe Log)")
                   .arg(dbPath).arg(error));
        return false;
    }
    log(LogEntry::System, QStringLiteral("SQLite-Export nach %1 geschrieben (inkl. modelle/materialien/wartungen)").arg(dbPath));
    return true;
}

static bool hasTable(QSqlQuery &q, const QString &name)
{
    q.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(name));
    return q.next();
}

bool DataStore::importSqlite(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("import"));
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        log(LogEntry::File, QStringLiteral("SQLite-Import fehlgeschlagen: %1").arg(db.lastError().text()));
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("import"));
        return false;
    }
    QSqlQuery q(db);

    if (!hasTable(q, QStringLiteral("drucker")) || !hasTable(q, QStringLiteral("auftraege"))) {
        log(LogEntry::File, QStringLiteral("SQLite-Import abgelehnt: Pflicht-Tabelle drucker/auftraege fehlt in %1").arg(dbPath));
        q = QSqlQuery();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("import"));
        return false;
    }

    m_printers.clear(); m_auftraege.clear();
    QList<DeviceModel> newModels;
    QList<Material> materialienNeu;
    QList<Maintenance> newMaintenances;

    QSet<QString> seenPrinters, seenJobs, seenModels, seenMaterials, seenMaintenances;
    auto addDefensively = [this](auto &liste, QSet<QString> &seen, const QString &table,
                                      const auto &eintrag, const auto &idVon) {
        const QString id = idVon(eintrag);
        if (seen.contains(id)) {
            for (auto &x : liste)
                if (idVon(x) == id) { x = eintrag; break; }
            log(LogEntry::Validation, QStringLiteral("SQLite-Import: Duplikat-ID „%1“ in Tabelle %2 – letzter Eintrag gewinnt (konsistent zu Export/Map)")
                       .arg(id, table));
        } else {
            seen.insert(id);
            liste.append(eintrag);
        }
    };
    if (q.exec("SELECT * FROM drucker")) {
        while (q.next()) {
            Printer d;
            d.printerId = q.value(0).toString(); d.modelId = q.value(1).toString();
            d.location = q.value(2).toString(); d.commissionedOn = CsvIo::parseDate(q.value(3).toString());
            d.operatingHours = q.value(4).toDouble(); d.nozzleMm = q.value(5).toDouble();
            d.firmware = q.value(6).toString(); d.pidKp = q.value(7).toDouble();
            d.pidKi = q.value(8).toDouble(); d.pidKd = q.value(9).toDouble(); d.status = q.value(10).toString();
            addDefensively(m_printers, seenPrinters, QStringLiteral("drucker"), d,
                               [](const Printer &x) { return x.printerId; });
        }
    }
    if (q.exec("SELECT * FROM auftraege")) {
        while (q.next()) {
            Job a;
            a.jobId = q.value(0).toString(); a.printerId = q.value(1).toString();
            a.materialId = q.value(2).toString(); a.part = q.value(3).toString();
            a.start = CsvIo::parseDateTime(q.value(4).toString()); a.ende = CsvIo::parseDateTime(q.value(5).toString());
            a.plannedMinutes = q.value(6).toDouble(); a.plannedFilamentG = q.value(7).toDouble();
            a.jobResult = q.value(8).toString(); a.errorCode = q.value(9).toString(); a.note = q.value(10).toString();
            addDefensively(m_auftraege, seenJobs, QStringLiteral("auftraege"), a,
                               [](const Job &x) { return x.jobId; });
        }
    }

    bool hasModels = hasTable(q, QStringLiteral("modelle"));
    bool hatMaterialien = hasTable(q, QStringLiteral("materialien"));
    bool hasMaintenances = hasTable(q, QStringLiteral("wartungen"));
    if (hasModels && q.exec("SELECT * FROM modelle")) {
        while (q.next()) {
            DeviceModel m;
            m.modelId = q.value(0).toString(); m.manufacturer = q.value(1).toString();
            m.description = q.value(2).toString(); m.kinematics = q.value(3).toString();
            m.buildXMm = q.value(4).toDouble(); m.buildYMm = q.value(5).toDouble();
            m.buildZMm = q.value(6).toDouble(); m.maxNozzleTempC = q.value(7).toDouble();
            m.maxBedTempC = q.value(8).toDouble(); m.nozzleHeaterW = q.value(9).toDouble();
            m.bedHeaterW = q.value(10).toDouble();
            addDefensively(newModels, seenModels, QStringLiteral("modelle"), m,
                               [](const DeviceModel &x) { return x.modelId; });
        }
    }
    if (hatMaterialien && q.exec("SELECT * FROM materialien")) {
        while (q.next()) {
            Material m;
            m.materialId = q.value(0).toString(); m.description = q.value(1).toString();
            m.nozzleTempC = q.value(2).toDouble(); m.bedTempC = q.value(3).toDouble();
            m.densityGCm3 = q.value(4).toDouble(); m.priceEurKg = q.value(5).toDouble();
            m.fanPercent = q.value(6).toDouble();
            addDefensively(materialienNeu, seenMaterials, QStringLiteral("materialien"), m,
                               [](const Material &x) { return x.materialId; });
        }
    }
    if (hasMaintenances && q.exec("SELECT * FROM wartungen")) {
        while (q.next()) {
            Maintenance w;
            w.maintenanceId = q.value(0).toString(); w.printerId = q.value(1).toString();
            w.date = CsvIo::parseDate(q.value(2).toString()); w.kind = q.value(3).toString();
            w.durationMin = q.value(4).toDouble(); w.note = q.value(5).toString();
            addDefensively(newMaintenances, seenMaintenances, QStringLiteral("wartungen"), w,
                               [](const Maintenance &x) { return x.maintenanceId; });
        }
    }
    q = QSqlQuery();
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(QStringLiteral("import"));
    if (hasModels) m_models = newModels;
    if (hatMaterialien) m_materialien = materialienNeu;
    if (hasMaintenances) m_maintenances = newMaintenances;

    m_modelMap.clear(); m_materialMap.clear(); m_printerMap.clear();
    for (const auto &m : m_models) m_modelMap.insert(m.modelId, &m);
    for (const auto &m : m_materialien) m_materialMap.insert(m.materialId, &m);
    for (const auto &d : m_printers) m_printerMap.insert(d.printerId, &d);

    validateConsistency();
    log(LogEntry::System, QStringLiteral("SQLite-Import aus %1: %2 Drucker, %3 Aufträge, %4 Modelle, %5 Materialien, %6 Wartungen")
               .arg(dbPath).arg(m_printers.size()).arg(m_auftraege.size())
               .arg(m_models.size()).arg(m_materialien.size()).arg(m_maintenances.size()));
    emit logChanged();
    emit dataLoaded();
    return true;
}

const DeviceModel *DataStore::model(const QString &id) const { return m_modelMap.value(id, nullptr); }
const Material *DataStore::material(const QString &id) const { return m_materialMap.value(id, nullptr); }
const Printer *DataStore::printer(const QString &id) const { return m_printerMap.value(id, nullptr); }

QList<Job> DataStore::jobsOfPrinter(const QString &printerId) const
{
    QList<Job> out;
    for (const auto &a : m_auftraege)
        if (a.printerId == printerId) out.append(a);
    return out;
}

bool DataStore::updatePrinter(const Printer &d)
{
    for (auto &x : m_printers) {
        if (x.printerId == d.printerId) {
            x = d;
            m_printerMap.insert(d.printerId, &x);
            log(LogEntry::System, QStringLiteral("Stammdaten geändert: Drucker %1 (Status=%2, Standort=%3)")
                       .arg(d.printerId, d.status, d.location));
            emit logChanged();
            return true;
        }
    }
    return false;
}

// Zentrales Log; wird auf kMaxLogEntries begrenzt, aufeinanderfolgende Duplikate werden nicht doppelt gehaengt.
void DataStore::log(LogEntry::Category category, const QString &message, const QString &jobId) const
{

    if (!m_log.isEmpty() && m_log.last().jobId == jobId && m_log.last().message == message)
        return;
    m_log.append({category, QDateTime::currentDateTime(), jobId, message});

    if (m_log.size() > kMaxLogEntries)
        m_log.erase(m_log.begin(), m_log.begin() + (m_log.size() - kMaxLogEntries));
    const_cast<DataStore*>(this)->logChanged();
}

double DataStore::successRateOfPrinter(const QString &printerId) const
{
    int fertig = 0, success = 0;
    for (const auto &a : m_auftraege) {
        if (a.printerId != printerId) continue;
        if (a.jobResult == QLatin1String("ausstehend")) continue;
        ++fertig;
        if (a.jobResult == QLatin1String("erfolgreich")) ++success;
    }
    return fertig == 0 ? -1.0 : double(success) / fertig;
}

double DataStore::filamentUsedGrams(const QString &printerId) const
{
    double sum = 0;
    for (const auto &a : m_auftraege)
        if (a.printerId == printerId && a.jobResult == QLatin1String("erfolgreich"))
            sum += a.plannedFilamentG;
    return sum;
}

double DataStore::materialCostEur(const QString &printerId) const
{
    double sum = 0;
    for (const auto &a : m_auftraege) {
        if (a.printerId != printerId || a.jobResult != QLatin1String("erfolgreich")) continue;
        if (const Material *m = m_materialMap.value(a.materialId))
            sum += a.plannedFilamentG / 1000.0 * m->priceEurKg;
    }
    return sum;
}

double DataStore::meanWeeklyConsumptionKg(const QString &materialId) const
{
    double gesamtG = 0;
    QDateTime von, bis;
    for (const auto &a : m_auftraege) {
        if (a.jobResult != QLatin1String("erfolgreich") || !a.start.isValid()) continue;
        if (a.materialId == materialId)
            gesamtG += a.plannedFilamentG;
        if (!von.isValid() || a.start < von) von = a.start;
        const QDateTime ende = a.ende.isValid() ? a.ende : a.start;
        if (!bis.isValid() || ende > bis) bis = ende;
    }
    if (gesamtG <= 0 || !von.isValid())
        return 0.0;

    const double wochen = qMax(1.0 / 7.0, double(von.date().daysTo(bis.date())) / 7.0);
    return gesamtG / 1000.0 / wochen;
}
