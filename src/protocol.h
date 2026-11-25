// Druckprotokolle (JSON): Master-Index, LRU-Cache, synchron/async Laden.
#pragma once

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

    int loadIndex(const QString &dataFolder);
    QString dataFolder() const { return m_dataFolder; }

    QString pathForJob(const QString &jobId) const;

    PrintLog load(const QString &jobId);

    void loadAsync(const QString &jobId);

    bool isCached(const QString &jobId) const;
    int cacheSize() const { return m_cache.size(); }

signals:
    void fertig(const QString &jobId);
    void error(const QString &jobId, const QString &message);

private:
    PrintLog parseProtocolJson(const QString &path, const QString &jobId) const;
    void putInCache(const QString &jobId, const PrintLog &p);

    QString m_dataFolder;
    QHash<QString, QString> m_index;

    QList<QString> m_lru;
    QHash<QString, std::shared_ptr<PrintLog>> m_cache;
    static constexpr int kCacheSize = 20;
};
