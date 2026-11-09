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
    // RFC-4180-artig: Felder mit Komma/Quote/Umbruch stehen in doppelten Quotes
    // ("" = escaped quote). Die vorliegenden Projektdaten haben keine Quotes,
    // bleiben also unverändert kompatibel.
    QStringList out;
    QString cur;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.size() && line[i + 1] == QLatin1Char('"')) {
                    cur += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur += c;
            }
        } else {
            if (c == QLatin1Char('"')) {
                inQuotes = true;
            } else if (c == QLatin1Char(',')) {
                out.append(cur);
                cur.clear();
            } else {
                cur += c;
            }
        }
    }
    out.append(cur);
    return out;
}

QString CsvIo::joinLine(const QStringList &fields)
{
    QStringList quoted;
    for (const auto &f : fields) {
        if (f.contains(QLatin1Char(',')) || f.contains(QLatin1Char('"')) || f.contains(QLatin1Char('\n')))
            quoted.append(QStringLiteral("\"%1\"").arg(QString(f).replace(QLatin1Char('"'), QStringLiteral("\"\""))));
        else
            quoted.append(f);
    }
    return quoted.join(QLatin1Char(','));
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
