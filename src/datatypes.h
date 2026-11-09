#pragma once
// Zentrale Datenstrukturen der PrintFarm-Anwendung.
// Alle Strukturen bilden exakt die Spalten der globalen CSV-Dateien ab.

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QVector>
#include <QList>
#include <QJsonObject>

struct Modell {
    QString modellId;
    QString hersteller;
    QString bezeichnung;
    QString kinematik;          // kartesisch | corexy | delta
    double bauraumXMm = 0, bauraumYMm = 0, bauraumZMm = 0;
    double maxDueseTempC = 0, maxBettTempC = 0;
    double heizleistungDueseW = 0, heizleistungBettW = 0;
};

struct Material {
    QString materialId;
    QString bezeichnung;
    double dueseTempC = 0, bettTempC = 0;
    double dichteGCm3 = 0;
    double preisEurKg = 0;
    double luefterProzent = 0;
};

struct Drucker {
    QString druckerId;
    QString modellId;
    QString standort;
    QDate inbetriebnahme;
    double betriebsstunden = 0;
    double dueseMm = 0;
    QString firmware;
    double pidKp = 0, pidKi = 0, pidKd = 0;
    QString status;             // bereit | druckt | wartung | defekt
};

struct Auftrag {
    QString auftragId;
    QString druckerId;
    QString materialId;
    QString bauteil;
    QDateTime start;            // invalid bei geplanten Aufträgen
    QDateTime ende;
    double dauerSollMin = 0;
    double filamentSollG = 0;
    QString ergebnis;           // erfolgreich | abgebrochen | fehlgeschlagen | ausstehend
    QString fehlercode;
    QString bemerkung;
};

struct Wartung {
    QString wartungId;
    QString druckerId;
    QDate datum;
    QString art;
    double dauerMin = 0;
    QString bemerkung;
};

// Ein Messpunkt einer Druckprotokoll-Phase.
struct Messpunkt {
    double zeitS = 0;
    double dueseIstC = 0;
    double bettIstC = 0;
    double dueseSollC = 0;      // nur Druckphase
    double heizleistungProzent = 0;
    double luefterProzent = 0;  // nur Druckphase
    int schicht = 0;            // nur Druckphase; 0 = Pause
    double extruderStromA = 0;  // nur Druckphase
};

struct Ereignis {
    double zeitS = 0;
    QString typ;                // druck_start | pause | fortsetzen | abbruch | druck_ende
    QString text;
};

// Vollständig geladenes Druckprotokoll eines Auftrags.
struct Protokoll {
    QString auftragId;
    QString druckerId;
    QString materialId;
    double dueseSollC = 0, bettSollC = 0;
    int schichtanzahl = 0;
    double abtastungAufheizenS = 1, abtastungDruckS = 30;
    QVector<Messpunkt> aufheizen;
    QVector<Messpunkt> druck;
    QVector<Ereignis> ereignisse;
    bool valid = false;
    QString fehler;             // falls JSON defekt
};

// Ergebnis der fachlichen Regelgüte-Auswertung (Aufheizphase + Druckphase).
struct Regelguete {
    bool verfuegbar = false;
    double anstiegszeitS = 0;        // 10 % -> 90 % der Sprunghöhe
    double ueberschwingenK = 0;      // Tmax - Tsoll
    double ueberschwingenProzent = 0;
    double ausregelzeitS = 0;         // dauerhaft im ±2-K-Band; -1 = nicht ausgeregelt
    bool ausgeregelt = false;
    double mittlereAbweichungK = 0;  // Druckphase ohne Pausen/Wiederaufheizen
    double maximaleAbweichungK = 0;
    QString bemerkung;
};

// Ein Eintrag im zentralen Fehlerlog (Tab 4).
struct LogEintrag {
    enum Kategorie { Import, Validierung, Datei, Protokoll, System } kategorie;
    QDateTime zeitpunkt;
    QString auftragId;      // optional, leer = global
    QString meldung;
};
