#pragma once
// Fachliche Auswertung der Regelgüte nach Aufgabenstellung:
// Anstiegszeit (10 % -> 90 %), Überschwingen (K und %), Ausregelzeit (±2 K)
// aus der Aufheizphase sowie mittlere/maximale Regelabweichung in der
// Druckphase (ohne Pausen und Wiederaufheizen bis |e| <= 3 K).
// Reine Funktionen -> direkt mit Qt Test prüfbar.

#include "datatypes.h"

class RegelgueteRechner {
public:
    // Aufheizphase: zeitSekunden und dueseIstC (gleiche Länge, aufsteigend).
    // tsoll = Solltemperatur, t0 = Starttemperatur (erster Messwert).
    static Regelguete werteAus(const QVector<Messpunkt> &aufheizen,
                               const QVector<Messpunkt> &druck,
                               double tsoll);

    // Einzelberechnungen (öffentlich für Tests):
    static double anstiegszeitS(const QVector<double> &zeit, const QVector<double> &ist,
                                double t0, double tsoll);
    static double ueberschwingenK(const QVector<double> &ist, double tsoll);
    // -1 wenn nie dauerhaft im ±2-K-Band
    static double ausregelzeitS(const QVector<double> &zeit, const QVector<double> &ist,
                                double tsoll, double bandK = 2.0);
    // Druckphase: e = Tsoll - Ist; Pausen (schicht == 0) und Wiederaufheizen
    // (bis |e| <= 3 K nach einer Pause) werden ausgeschlossen.
    static void regelabweichungDruck(const QVector<Messpunkt> &druck,
                                     double tsoll, double &mittel, double &maxi);
};
