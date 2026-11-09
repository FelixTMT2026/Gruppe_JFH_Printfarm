#include "protocol.h"
#include "csvio.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrent>

ProtocolLoader::ProtocolLoader(QObject *parent) : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<Protokoll>::finished, this, [this]() {
        const Protokoll p = m_watcher.result();
        if (p.valid) {
            cachePut(p.auftragId, p);
            emit fertig(p.auftragId);
        } else {
            emit fehler(p.auftragId, p.fehler);
        }
    });
}

int ProtocolLoader::loadIndex(const QString &dataFolder)
{
    m_dataFolder = dataFolder;
    m_index.clear();
    bool ok = false;
    QList<CsvError> errors;
    const auto rows = CsvIo::read(dataFolder + "/master_protokoll_index.csv", &ok, &errors);
    if (!ok)
        return 0;
    for (int i = 1; i < rows.size(); ++i) {
        const auto &r = rows[i];
        if (r.size() >= 2 && !r[0].isEmpty())
            m_index.insert(r[0], r[1]);
    }
    return m_index.size();
}

QString ProtocolLoader::pfadFuer(const QString &auftragId) const
{
    return m_index.value(auftragId);
}

bool ProtocolLoader::cached(const QString &auftragId) const
{
    return m_cache.contains(auftragId);
}

void ProtocolLoader::cachePut(const QString &auftragId, const Protokoll &p)
{
    m_lru.removeAll(auftragId);
    m_lru.append(auftragId);
    m_cache.insert(auftragId, std::make_shared<Protokoll>(p));
    while (m_lru.size() > kCacheSize) {
        const QString alt = m_lru.takeFirst();
        m_cache.remove(alt);
    }
}

static QVector<Messpunkt> parsePhase(const QJsonObject &phase, bool isDruck, Protokoll &out)
{
    QVector<Messpunkt> punkte;
    const QJsonArray zeit = phase["zeit_s"].toArray();
    const QJsonArray dueseIst = phase["duese_ist_c"].toArray();
    const QJsonArray bettIst = phase["bett_ist_c"].toArray();
    const QJsonArray heiz = phase["heizleistung_duese_prozent"].toArray();
    const QJsonArray dueseSoll = phase["duese_soll_c"].toArray();
    const QJsonArray luefter = phase["luefter_prozent"].toArray();
    const QJsonArray schicht = phase["schicht"].toArray();
    const QJsonArray strom = phase["extruder_strom_a"].toArray();
    const int n = zeit.size();
    punkte.reserve(n);
    for (int i = 0; i < n; ++i) {
        Messpunkt m;
        m.zeitS = zeit[i].toDouble();
        m.dueseIstC = i < dueseIst.size() ? dueseIst[i].toDouble() : 0;
        m.bettIstC = i < bettIst.size() ? bettIst[i].toDouble() : 0;
        m.heizleistungProzent = i < heiz.size() ? heiz[i].toDouble() : 0;
        if (isDruck) {
            m.dueseSollC = i < dueseSoll.size() ? dueseSoll[i].toDouble() : 0;
            m.luefterProzent = i < luefter.size() ? luefter[i].toDouble() : 0;
            m.schicht = i < schicht.size() ? schicht[i].toInt() : 0;
            m.extruderStromA = i < strom.size() ? strom[i].toDouble() : 0;
        }
        punkte.append(m);
    }
    if (isDruck)
        out.abtastungDruckS = phase["abtastung_s"].toDouble(30);
    else
        out.abtastungAufheizenS = phase["abtastung_s"].toDouble(1);
    return punkte;
}

Protokoll ProtocolLoader::parseJson(const QString &path, const QString &auftragId) const
{
    Protokoll p;
    p.auftragId = auftragId;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        p.fehler = QStringLiteral("Protokolldatei nicht lesbar: %1").arg(path);
        return p;
    }
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        p.fehler = QStringLiteral("Ungültiges JSON (%1): %2").arg(perr.errorString(), path);
        return p;
    }
    const QJsonObject root = doc.object();
    const QJsonObject auftrag = root["auftrag"].toObject();
    p.druckerId = auftrag["drucker_id"].toString();
    p.materialId = auftrag["material_id"].toString();
    p.dueseSollC = auftrag["duesentemp_soll_c"].toDouble();
    p.bettSollC = auftrag["betttemp_soll_c"].toDouble();
    p.schichtanzahl = auftrag["schichtanzahl"].toInt();
    p.aufheizen = parsePhase(root["aufheizen"].toObject(), false, p);
    p.druck = parsePhase(root["druck"].toObject(), true, p);
    for (const auto &ev : root["ereignisse"].toArray()) {
        const QJsonObject o = ev.toObject();
        p.ereignisse.append({o["zeit_s"].toDouble(), o["typ"].toString(), o["text"].toString()});
    }
    p.valid = !p.aufheizen.isEmpty();
    if (!p.valid)
        p.fehler = QStringLiteral("Protokoll enthält keine Aufheizphase: %1").arg(path);
    return p;
}

Protokoll ProtocolLoader::load(const QString &auftragId)
{
    if (auto it = m_cache.find(auftragId); it != m_cache.end()) {
        m_lru.removeAll(auftragId);
        m_lru.append(auftragId);
        return *it.value();
    }
    const QString rel = m_index.value(auftragId);
    if (rel.isEmpty()) {
        Protokoll p;
        p.auftragId = auftragId;
        p.fehler = QStringLiteral("Kein Protokoll im Master-Index für diesen Auftrag");
        return p;
    }
    Protokoll p = parseJson(m_dataFolder + "/" + rel, auftragId);
    if (p.valid)
        cachePut(auftragId, p);
    return p;
}

void ProtocolLoader::loadAsync(const QString &auftragId)
{
    if (m_cache.contains(auftragId)) {
        emit fertig(auftragId);
        return;
    }
    const QString dataFolder = m_dataFolder;
    const QString rel = m_index.value(auftragId);
    m_watcher.setFuture(QtConcurrent::run([this, dataFolder, rel, auftragId]() {
        if (rel.isEmpty()) {
            Protokoll p;
            p.auftragId = auftragId;
            p.fehler = QStringLiteral("Kein Protokoll im Master-Index für diesen Auftrag");
            return p;
        }
        return parseJson(dataFolder + "/" + rel, auftragId);
    }));
}
