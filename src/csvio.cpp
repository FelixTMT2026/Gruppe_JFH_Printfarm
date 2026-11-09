#include "csvio.h"
#include <QTextStream>
#include <QFileInfo>

QList<QStringList> CsvIo::read(const QString &path, bool *ok, QList<CsvError> *errors)
{
    QList<QStringList> rows;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        if (errors) errors->append({0, QStringLiteral("Datei nicht lesbar: %1").arg(path)});
        return rows;
    }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    int zeile = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        ++zeile;
        if (line.trimmed().isEmpty())
            continue;
        rows.append(splitLine(line));
    }
    if (ok) *ok = true;
    return rows;
}

bool CsvIo::write(const QString &path, const QList<QStringList> &rows, QString *errorOut)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorOut) *errorOut = QStringLiteral("Datei nicht schreibbar: %1").arg(path);
        return false;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    for (const auto &row : rows)
        out << joinLine(row) << "\r\n";
    return true;
}

QStringList CsvIo::splitLine(const QString &line)
{
    // Die Projektdaten enthalten keine eingebetteten Trennzeichen/Quotes.
    return line.split(QLatin1Char(','));
}

QString CsvIo::joinLine(const QStringList &fields)
{
    return fields.join(QLatin1Char(','));
}

QDateTime CsvIo::parseDateTime(const QString &s)
{
    return QDateTime::fromString(s.trimmed(), QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}

QDate CsvIo::parseDate(const QString &s)
{
    return QDate::fromString(s.trimmed(), QStringLiteral("yyyy-MM-dd"));
}

QString CsvIo::formatDateTime(const QDateTime &dt)
{
    return dt.isValid() ? dt.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"))
                        : QString();
}

QString CsvIo::formatDate(const QDate &d)
{
    return d.isValid() ? d.toString(QStringLiteral("yyyy-MM-dd")) : QString();
}
