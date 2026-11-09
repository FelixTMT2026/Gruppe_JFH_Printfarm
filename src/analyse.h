#pragma once
// Fachliche Zusatzlogik (eigene Erweiterungen + Erweiterungen aus der
// Aufgabenstellung). Reine Funktionen bzw. klar gekapselte Klassen.

#include "datatypes.h"
#include "datastore.h"
#include <QStringList>

// Fehlererkennung in Druckprotokollen (Erweiterung 6) und Markierung schlecht
// eingestellter Regler.
class Fehlererkennung {
public:
    struct Befunde {
        bool schlechtEingestellt = false;   // großes Überschwingen/Dauerschwingung/lange Ausregelzeit
        QStringList fehlerbilder;            // z. B. "Düse verstopft", "Heizung ausgefallen"
    };
    static Befunde analysiere(const Protokoll &p, const Regelguete &rg,
                              const Material *material);
};

// Kostenrechnung je Auftrag (eigene Erweiterung): Material + Maschinenstunden.
class Kostenrechnung {
public:
    struct Ergebnis { double materialEur = 0; double maschinenEur = 0; double gesamtEur = 0; };
    // Stundensatz als Annahme, dokumentierbar: 12 €/h Maschinenstundensatz.
    static Ergebnis auftrag(const Auftrag &a, const Material *mat,
                            double stundensatzEur = 12.0);
};

// Auftragsplanung (Erweiterung 7): Greedy nach frühestem Fertigstellungszeitpunkt.
class Planung {
public:
    struct Zuweisung { QString auftragId; QString druckerId; QDateTime von; QDateTime bis; };
    // Weist geplante Aufträge (ergebnis == "ausstehend") geeigneten Druckern zu.
    // Geeignet: Status "bereit", Modell erlaubt Materialtemperaturen.
    static QList<Zuweisung> greedy(const QList<Auftrag> &ausstehend,
                                   const QList<Drucker> &drucker,
                                   const DataStore &store,
                                   const QDateTime &startZeit);
};

// PID-Simulation (eigene Erweiterung): simuliert die Sprungantwort des
// Regelkreises u(t) = Kp*e + Ki*∫e + Kd*de/dt mit einfacher Strecke 1. Ordnung.
class PidSim {
public:
    struct Ergebnis {
        QVector<double> zeitS, istC;
        double ueberschwingenK = 0;
        double anstiegszeitS = 0;
    };
    static Ergebnis simuliere(double kp, double ki, double kd,
                              double tsoll, double t0,
                              double tauS = 25.0, double dtS = 1.0, double dauerS = 300.0);
};
