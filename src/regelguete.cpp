#include "regelguete.h"
#include <QtMath>

// Lineare Interpolation: Zeitpunkt, an dem die Kurve den Wert level erreicht.
static double zeitpunktBeiWert(const QVector<double> &zeit, const QVector<double> &ist,
                               double level, int startIdx)
{
    for (int i = qMax(1, startIdx); i < ist.size(); ++i) {
        const double a = ist[i - 1], b = ist[i];
        if ((a - level) * (b - level) <= 0 && qAbs(b - a) > 1e-9) {
            const double f = (level - a) / (b - a);
            return zeit[i - 1] + f * (zeit[i] - zeit[i - 1]);
        }
    }
    return -1.0;
}

double RegelgueteRechner::anstiegszeitS(const QVector<double> &zeit, const QVector<double> &ist,
                                        double t0, double tsoll)
{
    if (zeit.size() < 2 || tsoll <= t0)
        return 0.0;
    const double sprung = tsoll - t0;
    const double t10 = zeitpunktBeiWert(zeit, ist, t0 + 0.10 * sprung, 0);
    const double t90 = zeitpunktBeiWert(zeit, ist, t0 + 0.90 * sprung, 0);
    if (t10 < 0 || t90 < 0)
        return 0.0;
    return t90 - t10;
}

double RegelgueteRechner::ueberschwingenK(const QVector<double> &ist, double tsoll)
{
    if (ist.isEmpty())
        return 0.0;
    double tmax = ist.first();
    for (double v : ist)
        tmax = qMax(tmax, v);
    return tmax - tsoll;
}

double RegelgueteRechner::ausregelzeitS(const QVector<double> &zeit, const QVector<double> &ist,
                                        double tsoll, double bandK)
{
    if (zeit.isEmpty())
        return -1.0;
    // Ausregelzeit: Zeitpunkt, ab dem die Temperatur dauerhaft im Band bleibt.
    // Rückwärts suchen: letzter Zeitpunkt außerhalb des Bandes.
    int letzterAusreisser = -1;
    for (int i = 0; i < ist.size(); ++i)
        if (qAbs(ist[i] - tsoll) > bandK)
            letzterAusreisser = i;
    if (letzterAusreisser < 0)
        return zeit.first();           // nie außerhalb des Bandes
    if (letzterAusreisser == ist.size() - 1)
        return -1.0;                    // nie dauerhaft im Band (nicht ausgeregelt)
    return zeit[letzterAusreisser + 1];
}

void RegelgueteRechner::regelabweichungDruck(const QVector<Messpunkt> &druck,
                                             double tsoll, double &mittel, double &maxi)
{
    mittel = 0.0;
    maxi = 0.0;
    double sum = 0.0;
    qint64 n = 0;
    bool wiederaufheizen = false;   // nach Pause bis |e| <= 3 K ausschließen
    for (const auto &p : druck) {
        const double e = tsoll - p.dueseIstC;
        if (p.schicht == 0) {          // Pause: Messpunkt überspringen, Wiederaufheizen markieren
            wiederaufheizen = true;
            continue;
        }
        if (wiederaufheizen) {
            if (qAbs(e) <= 3.0)
                wiederaufheizen = false;    // erst ab |e| <= 3 K wieder mitzählen
            else
                continue;
        }
        sum += e;
        maxi = qMax(maxi, qAbs(e));
        ++n;
    }
    if (n > 0)
        mittel = sum / n;
}

Regelguete RegelgueteRechner::werteAus(const QVector<Messpunkt> &aufheizen,
                                       const QVector<Messpunkt> &druck,
                                       double tsoll)
{
    Regelguete r;
    if (aufheizen.size() < 2)
        return r;
    QVector<double> zeit, ist;
    zeit.reserve(aufheizen.size());
    ist.reserve(aufheizen.size());
    for (const auto &p : aufheizen) {
        zeit.append(p.zeitS);
        ist.append(p.dueseIstC);
    }
    const double t0 = ist.first();
    const double sprung = tsoll - t0;
    if (sprung <= 0) {
        r.bemerkung = QStringLiteral("Solltemperatur nicht über Starttemperatur");
        return r;
    }
    r.verfuegbar = true;
    r.anstiegszeitS = anstiegszeitS(zeit, ist, t0, tsoll);
    r.ueberschwingenK = ueberschwingenK(ist, tsoll);
    r.ueberschwingenProzent = sprung > 0 ? 100.0 * r.ueberschwingenK / sprung : 0.0;
    const double ta = ausregelzeitS(zeit, ist, tsoll, 2.0);
    r.ausgeregelt = ta >= 0;
    r.ausregelzeitS = ta;
    if (!r.ausgeregelt)
        r.bemerkung = QStringLiteral("Nicht ausgeregelt (nie dauerhaft im ±2-K-Band)");
    regelabweichungDruck(druck, tsoll, r.mittlereAbweichungK, r.maximaleAbweichungK);
    return r;
}
