// Regelguete: Anstiegszeit, Ueberschwingen, Ausregelzeit, Abweichung Druckphase.
#pragma once

#include "datatypes.h"

class ControlQualityCalculator {
public:

    static ControlQuality computeValues(const QVector<Sample> &heatUp,
                               const QVector<Sample> &printPhase,
                               double targetC);

    static double riseTimeS(const QVector<double> &times, const QVector<double> &ist,
                                double t0, double targetC);
    static double overshootK(const QVector<double> &ist, double targetC);

    static double settlingTimeS(const QVector<double> &times, const QVector<double> &ist,
                                double targetC, double bandK = 2.0);

    static void deviationDuringPrint(const QVector<Sample> &printPhase,
                                     double targetC, double &mittel, double &maxi);
};
