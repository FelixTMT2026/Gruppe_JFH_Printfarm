// CSV-Ein-/Ausgabe inkl. Formel-Injection-Schutz und Datumsfunktionen.
#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QFile>

struct CsvError {
    int line = 0;
    QString message;
};

class CsvIo {
public:

    static QList<QStringList> readCsv(const QString &path, bool *ok = nullptr,
                                   QList<CsvError> *errors = nullptr);

    static bool writeCsv(const QString &path, const QList<QStringList> &rows,
                      QString *errorOut = nullptr);

    static QStringList splitLine(const QString &line);
    static QString joinLine(const QStringList &fields);

    static QDateTime parseDateTime(const QString &s);
    static QDate parseDate(const QString &s);
    static QString formatDateTime(const QDateTime &dt);
    static QString formatDate(const QDate &d);
};
