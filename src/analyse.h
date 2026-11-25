// Fachlogik: Fehlererkennung in Protokollen, Kostenrechnung, Auftragsplanung.
#pragma once

#include "datatypes.h"
#include "datastore.h"
#include <QStringList>

class ErrorDetection {
public:
    struct Findings {
        bool poorlyTuned = false;
        QStringList errorPatterns;
    };
    static Findings detectFaults(const PrintLog &p, const ControlQuality &rg,
                              const Material *material);
};

class CostCalculator {
public:
    struct Result { double materialEur = 0; double maschinenEur = 0; double gesamtEur = 0; };

    static Result computeJobCost(const Job &a, const Material *mat,
                            double stundensatzEur = 12.0);
};

class Planning {
public:
    struct Assignment { QString jobId; QString printerId; QDateTime von; QDateTime bis; };

    static QList<Assignment> assignJobsGreedy(const QList<Job> &pending,
                                   const QList<Printer> &printer,
                                   const DataStore &store,
                                   const QDateTime &startTime);
};

