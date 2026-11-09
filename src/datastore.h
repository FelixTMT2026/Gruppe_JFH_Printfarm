#pragma once
// Zentraler Datenspeicher: lädt die globalen CSV-Dateien, hält alle Stammdaten
// im Speicher, führt Plausibilitätsprüfungen durch und exportiert CSV/SQLite.

#include "datatypes.h"
#include <QObject>
#include <QHash>
#include <QList>

class DataStore : public QObject {
    Q_OBJECT
public:
    explicit DataStore(QObject *parent = nullptr);

    // Lädt alle globalen CSVs aus dem Datenordner. Rückgabe: true, wenn die
    // Pflichtdateien (drucker/modelle/materialien/auftraege) geladen wurden.
    // Fehler landen in log() und im Fehlerlog.
    bool loadFromFolder(const QString &folder);
    QString dataFolder() const { return m_dataFolder; }

    // Export der (ggf. geänderten) Stammdaten als *_out.csv in gewählten Ordner.
    bool exportCsv(const QString &folder, QString *errorOut = nullptr) const;

    // Persistenz-Upgrade: SQLite-Import/Export (Qt SQL).
    bool exportSqlite(const QString &dbPath, QString *errorOut = nullptr) const;
    bool importSqlite(const QString &dbPath);

    const QList<Drucker> &drucker() const { return m_drucker; }
    const QList<Modell> &modelle() const { return m_modelle; }
    const QList<Material> &materialien() const { return m_materialien; }
    const QList<Auftrag> &auftraege() const { return m_auftraege; }
    const QList<Wartung> &wartungen() const { return m_wartungen; }
    const QList<LogEintrag> &log() const { return m_log; }

    const Modell *modell(const QString &id) const;
    const Material *material(const QString &id) const;
    const Drucker *drucker(const QString &id) const;
    QList<Auftrag> auftraegeOfDrucker(const QString &druckerId) const;

    // Stammdaten-Änderung (mit Rechteprüfung durch Aufrufer) + Logeintrag.
    bool updateDrucker(const Drucker &d);

    void addLog(LogEintrag::Kategorie kat, const QString &meldung,
                const QString &auftragId = QString()) const;

    // Statistik-Hilfen (Erweiterung 1).
    double erfolgsquoteDrucker(const QString &druckerId) const; // 0..1, -1 = keine
    double filamentverbrauchG(const QString &druckerId) const;   // Summe Soll
    double materialkostenEur(const QString &druckerId) const;

signals:
    void logChanged();
    void datenGeladen();   // nach erfolgreichem Laden/Import der Stammdaten

private:
    bool loadDrucker(const QString &path);
    bool loadModelle(const QString &path);
    bool loadMaterialien(const QString &path);
    bool loadAuftraege(const QString &path);
    bool loadWartungen(const QString &path);
    void plausibilitaet();   // gegenseitige Prüfungen, Ergebnis ins Log

    QString m_dataFolder;
    QList<Drucker> m_drucker;
    QList<Modell> m_modelle;
    QList<Material> m_materialien;
    QList<Auftrag> m_auftraege;
    QList<Wartung> m_wartungen;
    mutable QList<LogEintrag> m_log;
    QHash<QString, const Modell*> m_modellMap;
    QHash<QString, const Material*> m_materialMap;
    QHash<QString, const Drucker*> m_druckerMap;
};
