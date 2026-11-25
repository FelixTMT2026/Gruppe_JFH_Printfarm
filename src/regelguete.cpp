// Implementierung siehe regelguete.h.

#include "regelguete.h"
#include <QtMath>

static double timeAtLevel(const QVector<double> &times, const QVector<double> &ist,
                               double level, int startIdx)
{
    for (int i = qMax(1, startIdx); i < ist.size(); ++i) {
        const double a = ist[i - 1], b = ist[i];
        if ((a - level) * (b - level) <= 0 && qAbs(b - a) > 1e-9) {
            const double f = (level - a) / (b - a);
            return times[i - 1] + f * (times[i] - times[i - 1]);
        }
    }
    return -1.0;
}

double ControlQualityCalculator::riseTimeS(const QVector<double> &times, const QVector<double> &ist,
                                        double t0, double targetC)
{
    if (times.size() < 2 || targetC <= t0)
        return 0.0;
    const double sprung = targetC - t0;
    const double t10 = timeAtLevel(times, ist, t0 + 0.10 * sprung, 0);
    const double t90 = timeAtLevel(times, ist, t0 + 0.90 * sprung, 0);
    if (t10 < 0 || t90 < 0)
        return 0.0;
    return t90 - t10;
}

double ControlQualityCalculator::overshootK(const QVector<double> &ist, double targetC)
{
    if (ist.isEmpty())
        return 0.0;
    double tmax = ist.first();
    for (double v : ist)
        tmax = qMax(tmax, v);
    return tmax - targetC;
}

double ControlQualityCalculator::settlingTimeS(const QVector<double> &times, const QVector<double> &ist,
                                        double targetC, double bandK)
{
    if (times.isEmpty())
        return -1.0;

    int letzterAusreisser = -1;
    for (int i = 0; i < ist.size(); ++i)
        if (qAbs(ist[i] - targetC) > bandK)
            letzterAusreisser = i;
    if (letzterAusreisser < 0)
        return times.first();
    if (letzterAusreisser == ist.size() - 1)
        return -1.0;
    return times[letzterAusreisser + 1];
}

void ControlQualityCalculator::deviationDuringPrint(const QVector<Sample> &printPhase,
                                             double targetC, double &mittel, double &maxi)
{
    mittel = 0.0;
    maxi = 0.0;
    double sum = 0.0;
    qint64 n = 0;
    bool reheating = false;
    for (const auto &p : printPhase) {
        const double e = targetC - p.nozzleActualC;
        if (p.layer == 0) {
            reheating = true;
            continue;
        }
        if (reheating) {
            if (qAbs(e) <= 3.0)
                reheating = false;
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

ControlQuality ControlQualityCalculator::computeValues(const QVector<Sample> &heatUp,
                                       const QVector<Sample> &printPhase,
                                       double targetC)
{
    ControlQuality r;
    if (heatUp.size() < 2)
        return r;
    QVector<double> times, ist;
    times.reserve(heatUp.size());
    ist.reserve(heatUp.size());
    for (const auto &p : heatUp) {
        times.append(p.timeS);
        ist.append(p.nozzleActualC);
    }
    const double t0 = ist.first();
    const double sprung = targetC - t0;
    if (sprung <= 0) {
        r.note = QStringLiteral("Solltemperatur nicht über Starttemperatur");
        return r;
    }
    r.available = true;
    r.riseTimeS = riseTimeS(times, ist, t0, targetC);
    r.overshootK = overshootK(ist, targetC);
    r.overshootPercent = sprung > 0 ? 100.0 * r.overshootK / sprung : 0.0;
    const double ta = settlingTimeS(times, ist, targetC, 2.0);
    r.settled = ta >= 0;
    r.settlingTimeS = ta;
    if (!r.settled)
        r.note = QStringLiteral("Nicht ausgeregelt (nie dauerhaft im ±2-K-Band)");
    deviationDuringPrint(printPhase, targetC, r.meanDeviationK, r.maxDeviationK);
    return r;
}
