// Implementierung der Fachlogik siehe analyse.h.

#include "analyse.h"
#include <QtMath>
#include <algorithm>

static constexpr double kOvershootBadK = 5.0;
static constexpr double kSustainedOscillationAmpK = 2.0;
static constexpr double kSettlingTimeBadS = 300.0;
static constexpr double kCurrentRiseFactor = 1.5;
static constexpr double kCurrentRiseAbsA = 0.2;
static constexpr double kHeaterFullPowerPercent = 95.0;
static constexpr double kHeaterMinRiseK = 2.0;
static constexpr double kSensorNoiseK = 1.5;
static constexpr double kBedDeviationK = 10.0;

// Regelbasierte Heuristiken mit dokumentierten Schwellen (Konstanten oben);
// keine physikalische Diagnose, sondern Auffaelligkeiten im Protokoll.
ErrorDetection::Findings ErrorDetection::detectFaults(const PrintLog &p, const ControlQuality &rg,
                                                      const Material *material)
{
    Findings b;

    if (rg.available) {
        bool sustainedOscillation = false;
        if (p.heatUp.size() >= 20) {
            const int n = p.heatUp.size();
            const int m = qMax(2, n / 10);
            const double amp = qAbs(p.heatUp[n - 1].nozzleActualC - p.heatUp[n - 1 - m].nozzleActualC);
            sustainedOscillation = amp > kSustainedOscillationAmpK;
        }
        if (rg.overshootK > kOvershootBadK || sustainedOscillation ||
            (!rg.settled) || rg.settlingTimeS > kSettlingTimeBadS)
            b.poorlyTuned = true;
    }

    if (p.printPhase.size() >= 20) {
        const int n = p.printPhase.size();
        const double anfang = p.printPhase[n / 4].extruderCurrentA;
        const double ende = p.printPhase[n - 1].extruderCurrentA;
        if (ende > anfang * kCurrentRiseFactor && ende - anfang > kCurrentRiseAbsA)
            b.errorPatterns.append(QStringLiteral("Düse vermutlich verstopft (steigender Extruderstrom)"));
    }

    if (p.heatUp.size() >= 30) {
        const int n = p.heatUp.size();
        const int i0 = qMax(1, n / 10);
        const int i1 = qMax(i0 + 2, n / 3);
        bool volleLeistung = true;
        double rise = 0;
        for (int i = i0; i < i1; ++i) {
            if (p.heatUp[i].heaterPercent < kHeaterFullPowerPercent) volleLeistung = false;
            rise = p.heatUp[i].nozzleActualC - p.heatUp[i0].nozzleActualC;
        }
        if (volleLeistung && rise < kHeaterMinRiseK)
            b.errorPatterns.append(QStringLiteral("Heizung vermutlich ausgefallen (volle Leistung ohne Temperaturanstieg)"));
    }

    if (p.printPhase.size() >= 20) {
        double noise = 0;
        for (int i = 2; i < p.printPhase.size(); ++i)
            noise += qAbs(p.printPhase[i].nozzleActualC - p.printPhase[i - 1].nozzleActualC);
        if (noise / (p.printPhase.size() - 2) > kSensorNoiseK)
            b.errorPatterns.append(QStringLiteral("Temperaturfühler verrauscht (starkes Messrauschen)"));
    }

    if (material && qAbs(p.bedTargetC - material->bedTempC) > kBedDeviationK)
        b.errorPatterns.append(QStringLiteral("Betttemperatur weicht stark von Materialempfehlung ab (%1 vs. %2 °C)")
                               .arg(p.bedTargetC).arg(material->bedTempC));
    return b;
}

CostCalculator::Result CostCalculator::computeJobCost(const Job &a, const Material *mat,
                                                  double stundensatzEur)
{
    Result e;
    if (mat)
        e.materialEur = a.plannedFilamentG / 1000.0 * mat->priceEurKg;
    double durationMin = a.plannedMinutes;
    if (a.start.isValid() && a.ende.isValid())
        durationMin = a.start.secsTo(a.ende) / 60.0;
    e.maschinenEur = durationMin / 60.0 * stundensatzEur;
    e.gesamtEur = e.materialEur + e.maschinenEur;
    return e;
}

QList<Planning::Assignment> Planning::assignJobsGreedy(const QList<Job> &pending,
                                          const QList<Printer> &printer,
                                          const DataStore &store,
                                          const QDateTime &startTime)
{
    QList<Assignment> out;

    QHash<QString, QDateTime> freiAb;
    for (const auto &d : printer)
        freiAb.insert(d.printerId, startTime);
    for (const auto &a : store.auftraege()) {
        if (a.jobResult == QLatin1String("ausstehend") || !a.ende.isValid()) continue;
        auto it = freiAb.find(a.printerId);
        if (it != freiAb.end() && a.ende > it.value())
            it.value() = a.ende;
    }
    for (const auto &w : store.maintenances()) {

        const QDateTime barEnd = QDateTime(w.date, QTime(0, 0)).addSecs(qint64(w.durationMin * 60));
        auto it = freiAb.find(w.printerId);
        if (it != freiAb.end() && barEnd > it.value())
            it.value() = barEnd;
    }

    QList<Job> liste = pending;
    std::sort(liste.begin(), liste.end(), [](const Job &a, const Job &b) {
        return a.plannedMinutes < b.plannedMinutes;
    });
    for (const auto &a : liste) {
        const Material *mat = store.material(a.materialId);
        QString bestId;
        QDateTime bestFertig;
        for (const auto &d : printer) {
            if (d.status != QLatin1String("bereit")) continue;
            const DeviceModel *m = store.model(d.modelId);
            if (!m || !mat) continue;
            if (mat->nozzleTempC > m->maxNozzleTempC || mat->bedTempC > m->maxBedTempC)
                continue;
            const QDateTime von = freiAb.value(d.printerId);
            const QDateTime bis = von.addSecs(qint64(a.plannedMinutes * 60));
            if (bestId.isEmpty() || bis < bestFertig) {
                bestId = d.printerId;
                bestFertig = bis;
            }
        }
        if (!bestId.isEmpty()) {
            out.append({a.jobId, bestId, freiAb.value(bestId), bestFertig});
            freiAb.insert(bestId, bestFertig);
        }
    }
    return out;
}
