// Zentrale Datenhaltung: CSV/SQLite-Import/Export, Plausibilitaet, Log, Statistik.
#pragma once

#include "datatypes.h"
#include <QObject>
#include <QHash>
#include <QList>

class DataStore : public QObject {
    Q_OBJECT
public:
    explicit DataStore(QObject *parent = nullptr);

    bool loadFromFolder(const QString &folder);
    QString dataFolder() const { return m_dataFolder; }

    bool exportCsv(const QString &folder, QString *errorOut = nullptr) const;

    bool exportSqlite(const QString &dbPath, QString *errorOut = nullptr) const;
    bool importSqlite(const QString &dbPath);

    const QList<Printer> &printer() const { return m_printers; }
    const QList<DeviceModel> &models() const { return m_models; }
    const QList<Material> &materialien() const { return m_materialien; }
    const QList<Job> &auftraege() const { return m_auftraege; }
    const QList<Maintenance> &maintenances() const { return m_maintenances; }
    const QList<LogEntry> &log() const { return m_log; }

    const DeviceModel *model(const QString &id) const;
    const Material *material(const QString &id) const;
    const Printer *printer(const QString &id) const;
    QList<Job> jobsOfPrinter(const QString &printerId) const;

    bool updatePrinter(const Printer &d);

    static constexpr int kMaxLogEntries = 5000;

    void log(LogEntry::Category category, const QString &message,
                const QString &jobId = QString()) const;

    double successRateOfPrinter(const QString &printerId) const;
    double filamentUsedGrams(const QString &printerId) const;
    double materialCostEur(const QString &printerId) const;

    double meanWeeklyConsumptionKg(const QString &materialId) const;

signals:
    void logChanged();
    void dataLoaded();

private:
    bool loadPrinters(const QString &path);
    bool loadModels(const QString &path);
    bool loadMaterials(const QString &path);
    bool loadJobs(const QString &path);
    bool loadMaintenances(const QString &path);
    void validateConsistency();
    void checkDuplicateIds();
    void validatePrinters(int &error);
    void validateJobs(int &error);

    QString m_dataFolder;
    QList<Printer> m_printers;
    QList<DeviceModel> m_models;
    QList<Material> m_materialien;
    QList<Job> m_auftraege;
    QList<Maintenance> m_maintenances;
    mutable QList<LogEntry> m_log;
    QHash<QString, const DeviceModel*> m_modelMap;
    QHash<QString, const Material*> m_materialMap;
    QHash<QString, const Printer*> m_printerMap;
};
