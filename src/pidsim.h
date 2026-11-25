// PID-Simulation: physikalische Strecke (PT2+Totzeit), Regler, Presets/Fit.
#pragma once

#include "datatypes.h"
#include <QString>

class PidSim {
public:

    struct Options {
        double heaterPowerW = 40.0;
        double tmaxC = 400.0;

        double radiationShare = 0.25;
        double blockCapacityJK = 10.0;
        double sensorCapacityJK = 2.0;
        double koppplungWK = 0.5;
        double fanWK = 0.05;
        double fanPercent = 0.0;
        double deadTimeS = 3.0;
        double actuatorTauS = 1.0;
        double dtS = 1.0;
        double durationS = 300.0;
        double measurementResolutionK = 0.1;
        double outputResolution = 1.0;
        bool antiWindup = true;
        bool printPhaseDisturbance = false;
        double disturbanceW = 6.0;
        double printPhaseFromS = 180.0;
        bool sensorNoise = false;
        double noiseK = 0.15;
    };
    struct Result {
        QVector<double> timeS, actualC;
        QVector<double> outputPercent;
        double overshootK = 0;
        double riseTimeS = 0;
        double settlingTimeS = -1;
    };
    struct FitResult {
        Options opt;
        double sse = 0;
    };

    struct RankEntry {
        double kp = 0, ki = 0, kd = 0;
        Result res;
        double score = 0;
    };
    static Result simulateStepResponse(double kp, double ki, double kd,
                              double targetC, double t0,
                              const Options &opt);
    static Result simulateStepResponse(double kp, double ki, double kd,
                              double targetC, double t0);

    static Options plantPresetForModel(const QString &modelId);

    static Options estimatePlantFromHeatUp(const QVector<Sample> &heatUp, double targetC,
                                       const Options &basis);
    static Options estimatePlantFromHeatUp(const QVector<Sample> &heatUp, double targetC);

    static FitResult fitPlantFromHeatUp(const QVector<Sample> &heatUp, double targetC,
                                     const Options &basis);
    static FitResult fitPlantFromHeatUp(const QVector<Sample> &heatUp, double targetC);

    static QList<QVector<double>> varyParametersAround(double kp, double ki, double kd);

    static QList<RankEntry> rankParameterCandidates(const QList<QVector<double>> &candidates,
                                        double targetC, double t0, const Options &opt);
};

inline PidSim::Result PidSim::simulateStepResponse(double kp, double ki, double kd,
                                          double targetC, double t0)
{ return simulateStepResponse(kp, ki, kd, targetC, t0, Options()); }

inline PidSim::Options PidSim::estimatePlantFromHeatUp(const QVector<Sample> &heatUp, double targetC)
{ return estimatePlantFromHeatUp(heatUp, targetC, Options()); }

inline PidSim::FitResult PidSim::fitPlantFromHeatUp(const QVector<Sample> &heatUp, double targetC)
{ return fitPlantFromHeatUp(heatUp, targetC, Options()); }
