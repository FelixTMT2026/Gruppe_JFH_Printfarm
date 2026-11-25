// Zentrale Datenstrukturen (entspricht exakt den CSV-Spalten bzw. dem JSON-Format).
#pragma once

#include <QString>
#include <QDate>
#include <QDateTime>
#include <QVector>
#include <QList>

struct DeviceModel {
    QString modelId;
    QString manufacturer;
    QString description;
    QString kinematics;
    double buildXMm = 0, buildYMm = 0, buildZMm = 0;
    double maxNozzleTempC = 0, maxBedTempC = 0;
    double nozzleHeaterW = 0, bedHeaterW = 0;
};

struct Material {
    QString materialId;
    QString description;
    double nozzleTempC = 0, bedTempC = 0;
    double densityGCm3 = 0;
    double priceEurKg = 0;
    double fanPercent = 0;
};

struct Printer {
    QString printerId;
    QString modelId;
    QString location;
    QDate commissionedOn;
    double operatingHours = 0;
    double nozzleMm = 0;
    QString firmware;
    double pidKp = 0, pidKi = 0, pidKd = 0;
    QString status;
};

struct Job {
    QString jobId;
    QString printerId;
    QString materialId;
    QString part;
    QDateTime start;
    QDateTime ende;
    double plannedMinutes = 0;
    double plannedFilamentG = 0;
    QString jobResult;
    QString errorCode;
    QString note;
};

struct Maintenance {
    QString maintenanceId;
    QString printerId;
    QDate date;
    QString kind;
    double durationMin = 0;
    QString note;
};

struct Sample {
    double timeS = 0;
    double nozzleActualC = 0;
    double bedActualC = 0;
    double nozzleTargetC = 0;
    double heaterPercent = 0;
    double fanPercent = 0;
    int layer = 0;
    double extruderCurrentA = 0;
};

struct Event {
    double timeS = 0;
    QString type;
};

struct PrintLog {
    QString jobId;
    QString printerId;
    double nozzleTargetC = 0, bedTargetC = 0;
    QVector<Sample> heatUp;
    QVector<Sample> printPhase;
    QVector<Event> events;
    bool valid = false;
    QString error;
};

struct ControlQuality {
    bool available = false;
    double riseTimeS = 0;
    double overshootK = 0;
    double overshootPercent = 0;
    double settlingTimeS = 0;
    bool settled = false;
    double meanDeviationK = 0;
    double maxDeviationK = 0;
    QString note;
};

struct LogEntry {
    enum Category { Import, Validation, File, PrintLog, System } category;
    QDateTime timestamp;
    QString jobId;
    QString message;
};
