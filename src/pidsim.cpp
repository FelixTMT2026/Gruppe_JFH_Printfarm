// Implementierung siehe pidsim.h.

#include "pidsim.h"
#include "regelguete.h"
#include <QtMath>
#include <cmath>
#include <algorithm>

// Verluste kalibriert auf T_max: linearer Anteil + Strahlung ~T^4 (Kelvin),
// bei T_max geht genau die Heizleistung verloren.
static void computeLossCoefficients(const PidSim::Options &o, double t0C,
                                 double &kLin, double &kRad)
{
    const double s = qBound(0.0, o.radiationShare, 1.0);
    const double tmaxK = o.tmaxC + 273.15, t0K = t0C + 273.15;
    kLin = (1.0 - s) * o.heaterPowerW / (o.tmaxC - t0C);
    kRad = s * o.heaterPowerW / (std::pow(tmaxK, 4) - std::pow(t0K, 4));
}

static double lossPowerWatt(const PidSim::Options &o, double kLin, double kRad,
                        double t0C, double T1)
{
    const double tK = T1 + 273.15, t0K = t0C + 273.15;
    double v = kLin * (T1 - t0C) + kRad * (std::pow(tK, 4) - std::pow(t0K, 4));
    v += o.fanWK * (o.fanPercent / 100.0) * (T1 - t0C);
    return v;
}

static double conductanceAtTemperature(const PidSim::Options &o, double kLin, double kRad,
                      double TC)
{
    const double tK = TC + 273.15;
    return kLin + 4.0 * kRad * std::pow(tK, 3)
           + o.fanWK * (o.fanPercent / 100.0);
}

// Regelschleife: PT2-Strecke (Block + Fuehler) mit Totzeit-Ringpuffer auf u,
// Anti-Windup (conditional integration) und D-Anteil auf dem Messwert (kein Kick).
PidSim::Result PidSim::simulateStepResponse(double kp, double ki, double kd,
                                   double targetC, double t0,
                                   const Options &opt)
{

    Result e;
    const double dt = opt.dtS;
    const int n = int(opt.durationS / dt) + 1;
    e.timeS.reserve(n);
    e.actualC.reserve(n);
    e.outputPercent.reserve(n);

    const int delayN = qMax(1, int(opt.deadTimeS / dt + 0.5));
    QVector<double> puffer(delayN, 0.0);

    const double tfD = (kp > 1e-9) ? (kd / kp) / 10.0 : 10.0 * dt;

    double kLin, kRad;
    computeLossCoefficients(opt, t0, kLin, kRad);

    double T1 = t0, T2 = t0;
    double uAct = 0.0;
    double integral = 0.0, dFilt = 0.0, yPrev = t0;
    unsigned long lcg = 12345;
    double tmax = t0;
    for (int i = 0; i < n; ++i) {
        const double t = i * dt;

        double y = T2;
        if (opt.sensorNoise) {
            lcg = lcg * 1103515245ul + 12345ul;
            y += opt.noiseK * (2.0 * double((lcg >> 16) & 0x7FFF) / 32767.0 - 1.0);
        }
        if (opt.measurementResolutionK > 0)
            y = std::round(y / opt.measurementResolutionK) * opt.measurementResolutionK;
        const double err = targetC - y;

        const double dRaw = (i == 0) ? 0.0 : -(y - yPrev) / dt;
        dFilt += dt / (tfD + dt) * (dRaw - dFilt);

        double uRaw = kp * err + ki * integral + kd * dFilt;
        const bool skip = opt.antiWindup &&
                          ((uRaw > 100.0 && err > 0.0) || (uRaw < 0.0 && err < 0.0));
        if (!skip)
            integral += err * dt;
        uRaw = kp * err + ki * integral + kd * dFilt;
        double u = qBound(0.0, uRaw, 100.0);

        if (opt.outputResolution > 0)
            u = std::round(u / opt.outputResolution) * opt.outputResolution;

        uAct += dt / (opt.actuatorTauS + dt) * (u - uAct);
        const double uVerz = puffer.first();
        puffer.removeFirst();
        puffer.append(uAct);

        const double stoer = (opt.printPhaseDisturbance && t >= opt.printPhaseFromS) ? opt.disturbanceW : 0.0;

        T1 += (uVerz / 100.0 * opt.heaterPowerW - lossPowerWatt(opt, kLin, kRad, t0, T1) - stoer)
              / opt.blockCapacityJK * dt;
        T2 += opt.koppplungWK * (T1 - T2) / opt.sensorCapacityJK * dt;

        yPrev = y;
        e.timeS.append(t);
        e.actualC.append(T2);
        e.outputPercent.append(u);
        tmax = qMax(tmax, T2);
    }
    e.overshootK = tmax - targetC;
    e.riseTimeS = ControlQualityCalculator::riseTimeS(e.timeS, e.actualC, t0, targetC);
    e.settlingTimeS = ControlQualityCalculator::settlingTimeS(e.timeS, e.actualC, targetC);
    return e;
}

PidSim::Options PidSim::plantPresetForModel(const QString &modelId)
{

    struct P { const char *id; double heaterW; double tau1S; double deadTimeS; };
    static const P presets[] = {
        {"MD01", 40.0,  80.0, 4.0},
        {"MD02", 50.0, 110.0, 4.0},
        {"MD03", 50.0,  75.0, 3.0},
        {"MD04", 60.0, 120.0, 5.0},
        {"MD05", 40.0,  65.0, 3.0},
        {"MD06", 30.0,  55.0, 2.0},
        {"MD07", 80.0, 140.0, 5.0},
        {"MD08", 100.0, 95.0, 4.0},
    };
    Options o;
    for (const auto &p : presets) {
        if (modelId == QLatin1String(p.id)) {
            o.heaterPowerW = p.heaterW;
            o.deadTimeS = p.deadTimeS;

            o.tmaxC = 380.0 + 0.5 * p.heaterW;
            double kLin, kRad;
            computeLossCoefficients(o, 24.0, kLin, kRad);
            o.blockCapacityJK = p.tau1S * conductanceAtTemperature(o, kLin, kRad, 210.0);
            return o;
        }
    }

    o.tmaxC = 380.0 + 0.5 * o.heaterPowerW;
    return o;
}

QList<QVector<double>> PidSim::varyParametersAround(double kp, double ki, double kd)
{

    QList<QVector<double>> out;
    out.append({kp, ki, kd});
    for (double f : {0.5, 2.0}) {
        out.append({kp * f, ki, kd});
        out.append({kp, ki * f, kd});
        out.append({kp, ki, kd * f});
    }
    return out;
}

QList<PidSim::RankEntry> PidSim::rankParameterCandidates(const QList<QVector<double>> &candidates,
                                             double targetC, double t0, const Options &opt)
{
    QList<RankEntry> out;
    for (const auto &k : candidates) {
        RankEntry e;
        e.kp = k.value(0, 0.0);
        e.ki = k.value(1, 0.0);
        e.kd = k.value(2, 0.0);
        e.res = simulateStepResponse(e.kp, e.ki, e.kd, targetC, t0, opt);

        if (e.res.settlingTimeS >= 0)
            e.score = e.res.settlingTimeS + 10.0 * qMax(0.0, e.res.overshootK);
        else
            e.score = 1.0e6 + qMax(0.0, e.res.overshootK);
        out.append(e);
    }
    std::sort(out.begin(), out.end(),
              [](const RankEntry &a, const RankEntry &b) { return a.score < b.score; });
    return out;
}

PidSim::Options PidSim::estimatePlantFromHeatUp(const QVector<Sample> &heatUp, double targetC,
                                            const Options &basis)
{

    Options o = basis;
    if (heatUp.size() < 10)
        return o;
    const double t0 = heatUp.first().nozzleActualC;
    double tEnd = t0;
    for (const auto &m : heatUp)
        tEnd = qMax(tEnd, m.nozzleActualC);
    const double hub = qMin(tEnd - t0, targetC - t0);
    if (hub <= 1.0)
        return o;
    double maxSteigung = 0.0;
    for (int i = 1; i < heatUp.size(); ++i) {
        const double dt = heatUp[i].timeS - heatUp[i - 1].timeS;
        if (dt > 0)
            maxSteigung = qMax(maxSteigung, (heatUp[i].nozzleActualC - heatUp[i - 1].nozzleActualC) / dt);
    }
    if (maxSteigung > 1e-6) {
        const double tau = hub / maxSteigung;
        double kLin, kRad;
        computeLossCoefficients(o, t0, kLin, kRad);
        o.blockCapacityJK = tau * conductanceAtTemperature(o, kLin, kRad, targetC);
    }
    for (int i = 1; i < heatUp.size(); ++i) {
        if (heatUp[i].nozzleActualC - t0 > 0.01 * hub) {
            o.deadTimeS = qBound(2.0, heatUp[i].timeS - heatUp.first().timeS, 5.0);
            break;
        }
    }
    return o;
}

static double squaredErrorOnHeatUp(const QVector<Sample> &heatUp, const PidSim::Options &o,
                       double t0)
{
    if (heatUp.size() < 10)
        return 1e18;
    const double duration = heatUp.last().timeS - heatUp.first().timeS;
    PidSim::Options tmp = o;
    tmp.dtS = 1.0;

    const double dt = tmp.dtS;
    const int n = int(duration / dt) + 1;
    const int delayN = qMax(1, int(tmp.deadTimeS / dt + 0.5));
    QVector<double> puffer(delayN, 100.0);
    double kLin, kRad;
    computeLossCoefficients(tmp, t0, kLin, kRad);
    double T1 = t0, T2 = t0, uAct = 100.0;
    double sse = 0.0;
    int idx = 0;
    for (int i = 0; i < n && idx < heatUp.size(); ++i) {
        uAct += dt / (tmp.actuatorTauS + dt) * (100.0 - uAct);
        const double uVerz = puffer.first();
        puffer.removeFirst();
        puffer.append(uAct);
        T1 += (uVerz / 100.0 * tmp.heaterPowerW - lossPowerWatt(tmp, kLin, kRad, t0, T1))
              / tmp.blockCapacityJK * dt;
        T2 += tmp.koppplungWK * (T1 - T2) / tmp.sensorCapacityJK * dt;

        while (idx < heatUp.size() && heatUp[idx].timeS <= heatUp.first().timeS + i * dt) {
            const double d = T2 - heatUp[idx].nozzleActualC;
            sse += d * d;
            ++idx;
        }
    }
    return sse;
}

// Fit der Strecke (C1, Kopplung, Totzeit) per Koordinatensuche an die ECHTE
// Aufheizkurve - nur der offene Teil mit u = 100 % ist dafuer geeignet.
PidSim::FitResult PidSim::fitPlantFromHeatUp(const QVector<Sample> &heatUp, double targetC,
                                          const Options &basis)
{

    QVector<Sample> offen;
    for (const auto &m : heatUp) {
        if (!offen.isEmpty() && m.heaterPercent < 99.5)
            break;
        offen.append(m);
    }
    if (offen.size() < 10)
        offen = heatUp;
    FitResult best;
    best.opt = estimatePlantFromHeatUp(offen, targetC, basis);
    if (offen.size() < 10)
        return best;
    const double t0 = offen.first().nozzleActualC;
    best.sse = squaredErrorOnHeatUp(offen, best.opt, t0);
    static const double faktoren[] = {0.6, 0.8, 1.25, 1.6};
    for (int runde = 0; runde < 4; ++runde) {
        bool verbessert = false;

        for (double f : faktoren) {
            Options cand = best.opt;
            cand.blockCapacityJK = qMax(0.5, best.opt.blockCapacityJK * f);
            const double s = squaredErrorOnHeatUp(offen, cand, t0);
            if (s < best.sse) { best.sse = s; best.opt = cand; verbessert = true; }
        }

        for (double f : faktoren) {
            Options cand = best.opt;
            cand.koppplungWK = qMax(0.05, best.opt.koppplungWK * f);
            const double s = squaredErrorOnHeatUp(offen, cand, t0);
            if (s < best.sse) { best.sse = s; best.opt = cand; verbessert = true; }
        }

        for (double f : faktoren) {
            Options cand = best.opt;
            cand.deadTimeS = qBound(2.0, best.opt.deadTimeS * f, 5.0);
            const double s = squaredErrorOnHeatUp(offen, cand, t0);
            if (s < best.sse) { best.sse = s; best.opt = cand; verbessert = true; }
        }
        if (!verbessert)
            break;
    }
    return best;
}
