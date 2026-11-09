#pragma once
// Laden der Druckprotokolle: master_protokoll_index.csv bildet auftrag_id ->
// JSON-Pfad ab. Protokolle werden lazy geladen, in einem LRU-Cache (20 Einträge)
// gehalten und per QtConcurrent im Hintergrund geladen (Erweiterung 5).

#include "datatypes.h"
#include <QObject>
#include <QHash>
#include <QList>
#include <QFutureWatcher>
#include <memory>

class ProtocolLoader : public QObject {
    Q_OBJECT
public:
    explicit ProtocolLoader(QObject *parent = nullptr);

    // Lädt den Master-Index aus dem Datenordner. Rückgabe: Anzahl Einträge.
    int loadIndex(const QString &dataFolder);
    QString dataFolder() const { return m_dataFolder; }

    // Liefert den Pfad zur Protokolldatei eines Auftrags (relativ zum Datenordner)
    // oder leer, wenn kein Protokoll indiziert ist.
    QString pfadFuer(const QString &auftragId) const;

    // Synchrones Laden (für Tests und Fallback); cached das Ergebnis.
    Protokoll load(const QString &auftragId);

    // Asynchrones Laden im Hintergrund; Signal fertig(auftragId) bei Abschluss.
    void loadAsync(const QString &auftragId);

    bool cached(const QString &auftragId) const;

signals:
    void fertig(const QString &auftragId);
    void fehler(const QString &auftragId, const QString &meldung);

private:
    Protokoll parseJson(const QString &path, const QString &auftragId) const;
    void cachePut(const QString &auftragId, const Protokoll &p);

    QString m_dataFolder;
    QHash<QString, QString> m_index;          // auftrag_id -> relativer Pfad
    // LRU-Cache: Liste in Zugriffsreihenfolge (hinten = zuletzt genutzt)
    QList<QString> m_lru;
    QHash<QString, std::shared_ptr<Protokoll>> m_cache;
    static constexpr int kCacheSize = 20;
};
