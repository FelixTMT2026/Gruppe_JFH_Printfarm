// Implementierung siehe protocol.h.

#include "protocol.h"
#include "csvio.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrent>

static constexpr qint64 kMaxLogBytes = 20 * 1024 * 1024;

// Path-Traversal-Schutz (CWE-22): nur Pfade innerhalb des Datenordners zulassen.
static QString safeProtocolPath(const QString &dataFolder, const QString &rel)
{
    if (rel.isEmpty() || QDir::isAbsolutePath(rel))
        return {};
    auto norm = [](const QString &p) {
        return QDir::cleanPath(p).replace(QLatin1Char('\\'), QLatin1Char('/'));
    };
    const QString base = norm(QDir(dataFolder).canonicalPath());
    if (base.isEmpty())
        return {};
    const QString full = norm(QDir(dataFolder).absoluteFilePath(rel));
    if (full != base && !full.startsWith(base + QLatin1Char('/')))
        return {};
    return full;
}

ProtocolLoader::ProtocolLoader(QObject *parent) : QObject(parent) {}

// Master-Index laden: auftragId -> relativer Pfad der Protokoll-JSON.
int ProtocolLoader::loadIndex(const QString &dataFolder)
{
    m_dataFolder = dataFolder;
    m_index.clear();
    bool ok = false;
    QList<CsvError> errors;
    const auto rows = CsvIo::readCsv(dataFolder + "/master_protokoll_index.csv", &ok, &errors);
    if (!ok)
        return 0;
    for (int i = 1; i < rows.size(); ++i) {
        const auto &r = rows[i];
        if (r.size() >= 2 && !r[0].isEmpty())
            m_index.insert(r[0], r[1]);
    }
    return m_index.size();
}

QString ProtocolLoader::pathForJob(const QString &jobId) const
{
    return m_index.value(jobId);
}

bool ProtocolLoader::isCached(const QString &jobId) const
{
    return m_cache.contains(jobId);
}

void ProtocolLoader::putInCache(const QString &jobId, const PrintLog &p)
{
    m_lru.removeAll(jobId);
    m_lru.append(jobId);
    m_cache.insert(jobId, std::make_shared<PrintLog>(p));
    while (m_lru.size() > kCacheSize) {
        const QString original = m_lru.takeFirst();
        m_cache.remove(original);
    }
}

static QVector<Sample> parsePhaseJson(const QJsonObject &phase, bool isPrintPhase)
{
    QVector<Sample> punkte;
    const QJsonArray times = phase["zeit_s"].toArray();
    const QJsonArray nozzleActual = phase["duese_ist_c"].toArray();
    const QJsonArray bedActual = phase["bett_ist_c"].toArray();
    const QJsonArray heater = phase["heizleistung_duese_prozent"].toArray();
    const QJsonArray nozzleTarget = phase["duese_soll_c"].toArray();
    const QJsonArray fan = phase["luefter_prozent"].toArray();
    const QJsonArray layer = phase["schicht"].toArray();
    const QJsonArray current = phase["extruder_strom_a"].toArray();
    const int n = times.size();
    punkte.reserve(n);
    for (int i = 0; i < n; ++i) {
        Sample m;
        m.timeS = times[i].toDouble();
        m.nozzleActualC = i < nozzleActual.size() ? nozzleActual[i].toDouble() : 0;
        m.bedActualC = i < bedActual.size() ? bedActual[i].toDouble() : 0;
        m.heaterPercent = i < heater.size() ? heater[i].toDouble() : 0;
        if (isPrintPhase) {
            m.nozzleTargetC = i < nozzleTarget.size() ? nozzleTarget[i].toDouble() : 0;
            m.fanPercent = i < fan.size() ? fan[i].toDouble() : 0;
            m.layer = i < layer.size() ? layer[i].toInt() : 0;
            m.extruderCurrentA = i < current.size() ? current[i].toDouble() : 0;
        }
        punkte.append(m);
    }
    return punkte;
}

PrintLog ProtocolLoader::parseProtocolJson(const QString &path, const QString &jobId) const
{
    PrintLog p;
    p.jobId = jobId;
    QFile f(path);
    if (QFileInfo(path).size() > kMaxLogBytes) {
        p.error = QStringLiteral("Protokolldatei zu gross (> %1 MB), Laden abgelehnt: %2")
                       .arg(kMaxLogBytes / (1024 * 1024)).arg(path);
        return p;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        p.error = QStringLiteral("Protokolldatei nicht lesbar: %1").arg(path);
        return p;
    }
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        p.error = QStringLiteral("Ungültiges JSON (%1): %2").arg(perr.errorString(), path);
        return p;
    }
    const QJsonObject root = doc.object();
    const QJsonObject job = root["auftrag"].toObject();
    p.printerId = job["drucker_id"].toString();
    p.nozzleTargetC = job["duesentemp_soll_c"].toDouble();
    p.bedTargetC = job["betttemp_soll_c"].toDouble();
    p.heatUp = parsePhaseJson(root["aufheizen"].toObject(), false);
    p.printPhase = parsePhaseJson(root["druck"].toObject(), true);
    for (const auto &ev : root["ereignisse"].toArray()) {
        const QJsonObject o = ev.toObject();
        p.events.append({o["zeit_s"].toDouble(), o["typ"].toString()});
    }
    p.valid = !p.heatUp.isEmpty();
    if (!p.valid)
        p.error = QStringLiteral("Protokoll enthält keine Aufheizphase: %1").arg(path);
    return p;
}

PrintLog ProtocolLoader::load(const QString &jobId)
{
    if (auto it = m_cache.find(jobId); it != m_cache.end()) {
        m_lru.removeAll(jobId);
        m_lru.append(jobId);
        return *it.value();
    }
    const QString rel = m_index.value(jobId);
    if (rel.isEmpty()) {
        PrintLog p;
        p.jobId = jobId;
        p.error = QStringLiteral("Kein Protokoll im Master-Index für diesen Auftrag");
        return p;
    }
    const QString pfad = safeProtocolPath(m_dataFolder, rel);
    if (pfad.isEmpty()) {
        PrintLog p;
        p.jobId = jobId;
        p.error = QStringLiteral("Pfad aus dem Master-Index liegt ausserhalb des Datenordners - abgelehnt");
        return p;
    }
    PrintLog p = parseProtocolJson(pfad, jobId);
    if (p.valid)
        putInCache(jobId, p);
    return p;
}

void ProtocolLoader::loadAsync(const QString &jobId)
{
    if (m_cache.contains(jobId)) {
        emit fertig(jobId);
        return;
    }
    const QString dataFolder = m_dataFolder;
    const QString rel = m_index.value(jobId);

    auto *watcher = new QFutureWatcher<PrintLog>(this);
    connect(watcher, &QFutureWatcher<PrintLog>::finished, this, [this, watcher, jobId]() {
        const PrintLog p = watcher->result();
        watcher->deleteLater();
        if (p.valid) {
            putInCache(p.jobId, p);
            emit fertig(p.jobId);
        } else {
            emit error(p.jobId, p.error);
        }
    });
    watcher->setFuture(QtConcurrent::run([this, dataFolder, rel, jobId]() {
        if (rel.isEmpty()) {
            PrintLog p;
            p.jobId = jobId;
            p.error = QStringLiteral("Kein Protokoll im Master-Index für diesen Auftrag");
            return p;
        }
        const QString pfad = safeProtocolPath(dataFolder, rel);
        if (pfad.isEmpty()) {
            PrintLog p;
            p.jobId = jobId;
            p.error = QStringLiteral("Pfad aus dem Master-Index liegt ausserhalb des Datenordners - abgelehnt");
            return p;
        }
        return parseProtocolJson(pfad, jobId);
    }));
}
