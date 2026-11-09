#pragma once
// Robuster CSV-Reader/Writer: UTF-8, Komma als Trennzeichen, Punkt als
// Dezimaltrennzeichen. Fehlerhafte Zeilen werden nicht abgebrochen, sondern
// als Fehler gemeldet (Degradation statt Programmabbruch).

#include <QString>
#include <QStringList>
#include <QList>
#include <QFile>

struct CsvError {
    int zeile = 0;          // 1-basiert, 0 = Datei nicht lesbar
    QString meldung;
};

class CsvIo {
public:
    // Liest eine CSV-Datei. Rückgabe: Zeilen (jede Zeile = Liste der Felder).
    // Bei Lesefehler wird ok=false und errors befüllt.
    static QList<QStringList> read(const QString &path, bool *ok = nullptr,
                                   QList<CsvError> *errors = nullptr);
    // Schreibt eine CSV-Datei (UTF-8, Komma, CRLF wie die Originaldaten).
    static bool write(const QString &path, const QList<QStringList> &rows,
                      QString *errorOut = nullptr);

    // Zerlegt eine einzelne CSV-Zeile (einfaches Parsen, keine Quotes in den
    // vorliegenden Daten; trotzdem robust gegen Restkommas).
    static QStringList splitLine(const QString &line);
    static QString joinLine(const QStringList &fields);

    // Hilfsfunktionen für Datums-/Zeitformate der Aufgabenstellung.
    static QDateTime parseDateTime(const QString &s);   // JJJJ-MM-TT hh:mm:ss
    static QDate parseDate(const QString &s);           // JJJJ-MM-TT
    static QString formatDateTime(const QDateTime &dt);
    static QString formatDate(const QDate &d);
};
