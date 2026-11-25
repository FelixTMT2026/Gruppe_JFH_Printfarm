

#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <algorithm>
#include "csvio.h"
#include "datastore.h"
#include "tablemodels.h"
#include "protocol.h"
#include "regelguete.h"
#include "analyse.h"
#include "pidsim.h"
#include "auth.h"

class TestPrintFarm : public QObject {
    Q_OBJECT
private slots:
    void csvImportAndPlausibility();
    void csvExport();
    void qualityHeatUp();
    void deviationWithPause();
    void protocolLazyLoading();
    void rechte();
    void planningGreedy();
    void costCalculation();
    void pidSimulation();
    void sqlitePersistence();

    void brokenCsvLines();
    void missingFilesDegradation();
    void brokenJson();
    void protocolNotInIndex();
    void lruCacheLimited();
    void asyncLoadSignal();
    void qualityNotSettled();
    void qualityNoStep();
    void csvRoundtrip();
    void planningRespectsMaterialLimits();
    void planningAllPrintersBusy();
    void costWithoutMaterial();
    void pidSimNullParams();
    void pidSimOvershootWithAggressiveKi();
    void pidSimDeadTimeOscillates();
    void pidSimWindupBounded();
    void pidSimTmaxPhysicallyCapped();
    void pidSimOutputQuantized();
    void pidSimFitFromHeatUp();
    void updatePrinterInExport();
    void invalidLineFormats();

    void qualityOvershoot();
    void deviationOnlyPauses();
    void utf8Roundtrip();
    void protocolHeatUpOnly();
    void weeklyConsumptionPerMaterial();

    void pathTraversalRejected();
    void csvFormulaInjectionBlocked();

    void pidRankingSorted();
    void pidSimUssInRealRange();
    void plausibilityExtended();
    void sqliteImportMasterDataRoundtrip();
    void sqliteExportDuplicates();
    void planningFreeAfterLastJob();
    void errorDetectionHeaterFailed();
    void errorDetectionBedTempFormat();

    void logCappedWithoutConsecutiveDuplicates();
    void logModelCachedIndexAboveCap();
    void materialSortWithManyMaterials();

    void invalidDateIsLogged();
    void sqliteImportWithoutRequiredTables();
};

static QString writeTestDatabase()
{
    const QString dir = QDir::tempPath() + "/printfarm_testdaten";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","Test","T-100","kartesisch","100","100","100","260","100","40","100"},
    });
    CsvIo::writeCsv(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20.0","100"},
    });
    CsvIo::writeCsv(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm",
         "firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-01-1","2024-01-01","100","0.4","1.0","5.0","0.1","2.0","bereit"},
        {"D002","MD99","A-01-2","2024-01-01","50","0.4","1.0","5.0","0.1","2.0","bereit"},
    });
    CsvIo::writeCsv(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min",
         "filament_soll_g","ergebnis","fehlercode","bemerkung"},
        {"J00001","D001","MAT01","Teil1","2026-01-01 08:00:00","2026-01-01 09:00:00","60","30","erfolgreich","",""},
        {"J00002","D001","MAT01","Teil2","","","120","50","ausstehend","",""},
        {"J00003","D002","MAT01","Teil3","2026-01-02 08:00:00","2026-01-02 08:30:00","30","10","fehlgeschlagen","THERMAL_RUNAWAY",""},
    });
    CsvIo::writeCsv(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},
        {"W0001","D001","2026-01-03","reinigung","20","ok"},
    });
    CsvIo::writeCsv(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"J00001","prot/J00001.json","10","2026-01-01 08:00:00","2026-01-01 09:00:00"},
    });
    QDir().mkpath(dir + "/prot");

    QJsonObject root;
    QJsonObject job;
    job["auftrag_id"] = "J00001"; job["drucker_id"] = "D001"; job["material_id"] = "MAT01";
    job["duesentemp_soll_c"] = 210.0; job["betttemp_soll_c"] = 60.0; job["schichtanzahl"] = 10;
    root["auftrag"] = job;
    QJsonObject auf;
    auf["abtastung_s"] = 1.0;
    QJsonArray z, di, bi, hz;
    for (int i = 0; i <= 100; ++i) {
        z.append(i);
        di.append(24.0 + (210.0 - 24.0) * i / 100.0);
        bi.append(24.0 + (60.0 - 24.0) * i / 100.0);
        hz.append(100.0);
    }
    auf["zeit_s"] = z; auf["duese_ist_c"] = di; auf["bett_ist_c"] = bi; auf["heizleistung_duese_prozent"] = hz;
    root["aufheizen"] = auf;
    QJsonObject dr;
    dr["abtastung_s"] = 30.0;
    QJsonArray z2, ds, di2, bi2, hz2, lu, sch, st;
    for (int i = 0; i < 20; ++i) {
        z2.append(120.0 + 30 * i);
        ds.append(210.0);
        const bool pause = (i >= 10 && i < 13);
        di2.append(pause ? 150.0 : 210.5);
        bi2.append(60.0);
        hz2.append(pause ? 80.0 : 45.0);
        lu.append(50.0);
        sch.append(pause ? 0 : i + 1);
        st.append(0.5);
    }
    dr["zeit_s"] = z2; dr["duese_soll_c"] = ds; dr["duese_ist_c"] = di2; dr["bett_ist_c"] = bi2;
    dr["heizleistung_duese_prozent"] = hz2; dr["luefter_prozent"] = lu; dr["schicht"] = sch;
    dr["extruder_strom_a"] = st;
    root["druck"] = dr;
    root["ereignisse"] = QJsonArray{ QJsonObject{{"zeit_s",120.0},{"typ","druck_start"}},
                                     QJsonObject{{"zeit_s",420.0},{"typ","pause"}},
                                     QJsonObject{{"zeit_s",510.0},{"typ","fortsetzen"}},
                                     QJsonObject{{"zeit_s",690.0},{"typ","druck_ende"}} };
    QFile f(dir + "/prot/J00001.json");
    f.open(QIODevice::WriteOnly);
    f.write(QJsonDocument(root).toJson());
    f.close();
    return dir;
}

void TestPrintFarm::csvImportAndPlausibility()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.printer().size(), 2);
    QCOMPARE(store.models().size(), 1);
    QCOMPARE(store.materialien().size(), 1);
    QCOMPARE(store.auftraege().size(), 3);
    QCOMPARE(store.maintenances().size(), 1);

    bool found = false;
    for (const auto &e : store.log())
        if (e.category == LogEntry::Validation && e.message.contains("MD99"))
            found = true;
    QVERIFY(found);
}

void TestPrintFarm::csvExport()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    const QString out = QDir::tempPath() + "/printfarm_export";
    QDir().mkpath(out);
    QString err;
    QVERIFY(store.exportCsv(out, &err));
    bool ok = false;
    QList<CsvError> errors;
    const auto rows = CsvIo::readCsv(out + "/drucker_out.csv", &ok, &errors);
    QVERIFY(ok);
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[1][0], QStringLiteral("D001"));
}

void TestPrintFarm::qualityHeatUp()
{

    QVector<Sample> auf, dr;
    QVector<double> z, ist;
    for (int i = 0; i <= 100; ++i) {
        Sample m; m.timeS = i; m.nozzleActualC = 24.0 + 186.0 * i / 100.0;
        auf.append(m); z.append(i); ist.append(m.nozzleActualC);
    }
    const ControlQuality rg = ControlQualityCalculator::computeValues(auf, dr, 210.0);
    QVERIFY(rg.available);
    QVERIFY(qAbs(rg.riseTimeS - 80.0) < 1.0);
    QCOMPARE(rg.overshootK, 0.0);
    QVERIFY(rg.settled);
    QVERIFY(rg.settlingTimeS > 0);
}

void TestPrintFarm::deviationWithPause()
{

    QVector<Sample> printPhase;
    for (int i = 0; i < 20; ++i) {
        Sample m; m.timeS = 120.0 + 30 * i; m.nozzleTargetC = 210.0;
        const bool pause = (i >= 10 && i < 13);
        m.nozzleActualC = pause ? 150.0 : 209.0;
        m.layer = pause ? 0 : i + 1;
        printPhase.append(m);
    }
    double mittel, maxi;
    ControlQualityCalculator::deviationDuringPrint(printPhase, 210.0, mittel, maxi);
    QVERIFY(qAbs(mittel - 1.0) < 1e-9);
    QVERIFY(qAbs(maxi - 1.0) < 1e-9);
}

void TestPrintFarm::protocolLazyLoading()
{
    const QString dir = writeTestDatabase();
    ProtocolLoader loader;
    QCOMPARE(loader.loadIndex(dir), 1);
    QCOMPARE(loader.pathForJob("J00001"), QStringLiteral("prot/J00001.json"));
    QVERIFY(loader.pathForJob("J99999").isEmpty());
    const PrintLog p = loader.load("J00001");
    QVERIFY(p.valid);
    QCOMPARE(p.heatUp.size(), 101);
    QCOMPARE(p.printPhase.size(), 20);
    QCOMPARE(p.events.size(), 4);
    QVERIFY(loader.isCached("J00001"));

    const PrintLog p2 = loader.load("J99999");
    QVERIFY(!p2.valid);
    QVERIFY(!p2.error.isEmpty());
}

void TestPrintFarm::rechte()
{

    Auth::logout();
    QVERIFY(!Auth::mayEditPid());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(!Auth::mayPlanJobs());
    QVERIFY(!Auth::mayExport());

    QVERIFY(!Auth::login(Auth::Admin, "a1", QStringLiteral("falsch")));
    QCOMPARE(Auth::role(), Auth::None);
    QVERIFY(!Auth::mayExport());

    QVERIFY(Auth::login(Auth::Operator, "b1", QStringLiteral("bediener123")));
    QVERIFY(!Auth::mayEditPid());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(!Auth::mayPlanJobs());
    QVERIFY(Auth::mayExport());

    QVERIFY(Auth::login(Auth::Engineer, "t1", QStringLiteral("technik123")));
    QVERIFY(Auth::mayEditPid());
    QVERIFY(Auth::mayPlanJobs());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(Auth::mayExport());

    QVERIFY(Auth::login(Auth::Admin, "a1", QStringLiteral("admin123")));
    QVERIFY(Auth::mayEditPid());
    QVERIFY(Auth::mayPlanJobs());
    QVERIFY(Auth::mayEditMasterData());
    QVERIFY(Auth::mayExport());

    Auth::logout();
    QVERIFY(!Auth::mayExport());
    QVERIFY(!Auth::mayEditPid());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(!Auth::mayPlanJobs());
}

void TestPrintFarm::planningGreedy()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QList<Job> pending;
    for (const auto &a : store.auftraege())
        if (a.jobResult == "ausstehend") pending.append(a);
    QCOMPARE(pending.size(), 1);
    const auto res = Planning::assignJobsGreedy(pending, store.printer(), store, QDateTime(QDate(2026,1,5), QTime(8,0)));
    QCOMPARE(res.size(), 1);
    QCOMPARE(res[0].printerId, QStringLiteral("D001"));
    QCOMPARE(res[0].jobId, QStringLiteral("J00002"));
}

void TestPrintFarm::costCalculation()
{
    Job a;
    a.jobId = "J1"; a.plannedFilamentG = 2000; a.plannedMinutes = 120;
    a.start = QDateTime(QDate(2026,1,1), QTime(8,0));
    a.ende = QDateTime(QDate(2026,1,1), QTime(10,0));
    Material m; m.priceEurKg = 20.0;
    const auto k = CostCalculator::computeJobCost(a, &m, 12.0);
    QVERIFY(qAbs(k.materialEur - 40.0) < 1e-9);
    QVERIFY(qAbs(k.maschinenEur - 24.0) < 1e-9);
    QVERIFY(qAbs(k.gesamtEur - 64.0) < 1e-9);
}

void TestPrintFarm::pidSimulation()
{
    const auto res = PidSim::simulateStepResponse(5.0, 0.1, 2.0, 210.0, 24.0);
    QVERIFY(res.timeS.size() > 100);
    QVERIFY(res.riseTimeS > 0);

    QVERIFY(qAbs(res.actualC.last() - 210.0) < 5.0);
}

void TestPrintFarm::sqlitePersistence()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    const QString dbPath = QDir::tempPath() + "/printfarm_test.sqlite";
    QFile::remove(dbPath);
    QString err;
    QVERIFY(store.exportSqlite(dbPath, &err));
    DataStore store2;
    QVERIFY(store2.importSqlite(dbPath));
    QCOMPARE(store2.printer().size(), 2);
    QCOMPARE(store2.auftraege().size(), 3);
}

void TestPrintFarm::brokenCsvLines()
{
    const QString dir = QDir::tempPath() + "/pf_kaputt";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","T","T-1","kartesisch","1","1","1","260","100","40","100"},
        {"MD02","T","T-2"},
        {"","T","T-3","delta","1","1","1","260","100","40","100"},
    });
    CsvIo::writeCsv(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20","100"},
    });
    CsvIo::writeCsv(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm","firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-1","2024-13-99","100","0.4","1.0","5","0.1","2","bereit"},
        {"D002","MD01","A-2","2024-01-01","abc","0.4","1.0","5","0.1","2","bereit"},
    });
    CsvIo::writeCsv(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min","filament_soll_g","ergebnis","fehlercode","bemerkung"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.models().size(), 1);
    QCOMPARE(store.printer().size(), 2);

    bool logFound = false;
    for (const auto &e : store.log())
        if (e.message.contains("modelle.csv"))
            logFound = true;
    QVERIFY(logFound);

    bool dateLogged = false;
    for (const auto &e : store.log())
        if (e.category == LogEntry::Validation && e.message.contains(QString::fromUtf8("ungültiges Datum"))
            && e.message.contains(QStringLiteral("2024-13-99")))
            dateLogged = true;
    QVERIFY(dateLogged);
    QVERIFY(!store.printer(QStringLiteral("D001"))->commissionedOn.isValid());
}

void TestPrintFarm::missingFilesDegradation()
{
    const QString dir = QDir::tempPath() + "/pf_leer";
    QDir().mkpath(dir);
    DataStore store;
    QVERIFY(!store.loadFromFolder(dir));
    QCOMPARE(store.printer().size(), 0);
    bool fileErrors = false;
    for (const auto &e : store.log())
        if (e.category == LogEntry::File) fileErrors = true;
    QVERIFY(fileErrors);
}

void TestPrintFarm::brokenJson()
{
    const QString dir = writeTestDatabase();

    QFile f(dir + "/prot/defekt.json");
    f.open(QIODevice::WriteOnly);
    f.write("{ das ist kein json !!!");
    f.close();
    CsvIo::writeCsv(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"J00001","prot/J00001.json","10","2026-01-01 08:00:00","2026-01-01 09:00:00"},
        {"J00002","prot/defekt.json","5","2026-01-01 08:00:00","2026-01-01 09:00:00"},
        {"J00003","prot/fehlt.json","5","2026-01-01 08:00:00","2026-01-01 09:00:00"},
    });
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const PrintLog bad = loader.load("J00002");
    QVERIFY(!bad.valid);
    QVERIFY(bad.error.contains(QStringLiteral("JSON")));
    const PrintLog missing = loader.load("J00003");
    QVERIFY(!missing.valid);
    QVERIFY(missing.error.contains(QStringLiteral("nicht lesbar")));
}

void TestPrintFarm::protocolNotInIndex()
{
    const QString dir = writeTestDatabase();
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const PrintLog p = loader.load("J99999");
    QVERIFY(!p.valid);
    QVERIFY(p.error.contains(QStringLiteral("Master-Index")));
}

void TestPrintFarm::lruCacheLimited()
{
    const QString dir = writeTestDatabase();

    QList<QStringList> rows = {{"auftrag_id","pfad","anzahl_werte","start","ende"}};
    for (int i = 0; i < 25; ++i)
        rows.append({QStringLiteral("J%1").arg(i, 5, 10, QLatin1Char('0')), "prot/J00001.json", "10", "", ""});
    CsvIo::writeCsv(dir + "/master_protokoll_index.csv", rows);
    ProtocolLoader loader;
    loader.loadIndex(dir);
    for (int i = 0; i < 25; ++i)
        loader.load(QStringLiteral("J%1").arg(i, 5, 10, QLatin1Char('0')));
    QCOMPARE(loader.cacheSize(), 20);
    QVERIFY(loader.isCached(QStringLiteral("J00024")));
    QVERIFY(!loader.isCached(QStringLiteral("J00000")));
}

void TestPrintFarm::asyncLoadSignal()
{
    const QString dir = writeTestDatabase();
    ProtocolLoader loader;
    loader.loadIndex(dir);
    QSignalSpy spy(&loader, &ProtocolLoader::fertig);
    loader.loadAsync("J00001");
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.count(), 1);
    QVERIFY(loader.isCached("J00001"));

    QSignalSpy spy2(&loader, &ProtocolLoader::fertig);
    loader.loadAsync("J00001");
    QCOMPARE(spy2.count(), 1);
}

void TestPrintFarm::qualityNotSettled()
{

    QVector<Sample> auf, dr;
    for (int i = 0; i <= 50; ++i) {
        Sample m; m.timeS = i; m.nozzleActualC = 100.0;
        auf.append(m);
    }
    const ControlQuality rg = ControlQualityCalculator::computeValues(auf, dr, 210.0);
    QVERIFY(rg.available);
    QVERIFY(!rg.settled);
    QCOMPARE(rg.settlingTimeS, -1.0);
}

void TestPrintFarm::qualityNoStep()
{

    QVector<Sample> auf, dr;
    for (int i = 0; i <= 10; ++i) {
        Sample m; m.timeS = i; m.nozzleActualC = 210.0;
        auf.append(m);
    }
    const ControlQuality rg = ControlQualityCalculator::computeValues(auf, dr, 210.0);
    QVERIFY(!rg.available);
    QVERIFY(!rg.note.isEmpty());
}

void TestPrintFarm::csvRoundtrip()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));

    Printer d = store.printer()[0];
    d.status = QStringLiteral("defekt");
    d.location = QStringLiteral("B-99-9");
    QVERIFY(store.updatePrinter(d));
    const QString out = QDir::tempPath() + "/pf_roundtrip";
    QDir().mkpath(out);

    QFile::copy(dir + "/modelle.csv", out + "/modelle.csv");
    QFile::copy(dir + "/materialien.csv", out + "/materialien.csv");
    QString err;
    QVERIFY(store.exportCsv(out, &err));

    DataStore store2;

    QFile::remove(out + "/drucker.csv");
    QFile::rename(out + "/drucker_out.csv", out + "/drucker.csv");
    QFile::remove(out + "/auftraege.csv");
    QFile::rename(out + "/auftraege_out.csv", out + "/auftraege.csv");
    QVERIFY(store2.loadFromFolder(out));
    const Printer *d2 = store2.printer(store.printer()[0].printerId);
    QVERIFY(d2);
    QCOMPARE(d2->status, QStringLiteral("defekt"));
    QCOMPARE(d2->location, QStringLiteral("B-99-9"));
    QCOMPARE(store2.auftraege().size(), store.auftraege().size());
}

void TestPrintFarm::planningRespectsMaterialLimits()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));

    Material heiss;
    heiss.materialId = "MAT99"; heiss.description = "PEEK"; heiss.nozzleTempC = 300; heiss.bedTempC = 120;
    Job a;
    a.jobId = "JHEISS"; a.printerId = ""; a.materialId = "MAT99";
    a.plannedMinutes = 60; a.jobResult = QStringLiteral("ausstehend");

    auto res = Planning::assignJobsGreedy({a}, store.printer(), store, QDateTime(QDate(2026,1,5), QTime(8,0)));
    QCOMPARE(res.size(), 0);
}

void TestPrintFarm::planningAllPrintersBusy()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QList<Printer> alle = store.printer();
    for (auto &d : alle) d.status = QStringLiteral("defekt");
    Job a;
    a.jobId = "JX"; a.materialId = "MAT01"; a.plannedMinutes = 30;
    a.jobResult = QStringLiteral("ausstehend");
    auto res = Planning::assignJobsGreedy({a}, alle, store, QDateTime(QDate(2026,1,5), QTime(8,0)));
    QCOMPARE(res.size(), 0);
}

void TestPrintFarm::costWithoutMaterial()
{
    Job a;
    a.plannedFilamentG = 1000; a.plannedMinutes = 60;
    const auto k = CostCalculator::computeJobCost(a, nullptr);
    QCOMPARE(k.materialEur, 0.0);
    QVERIFY(k.gesamtEur > 0);
}

void TestPrintFarm::pidSimNullParams()
{
    const auto res = PidSim::simulateStepResponse(0, 0, 0, 210.0, 24.0);
    QVERIFY(res.timeS.size() > 10);
    QVERIFY(qAbs(res.actualC.last() - 24.0) < 1.0);
}

void TestPrintFarm::pidSimOvershootWithAggressiveKi()
{

    const auto res = PidSim::simulateStepResponse(6.0, 1.5, 0.0, 210.0, 24.0);
    QVERIFY(res.overshootK > 0.0);
    QVERIFY(res.outputPercent.size() == res.timeS.size());
}

void TestPrintFarm::pidSimDeadTimeOscillates()
{

    PidSim::Options opt;
    opt.deadTimeS = 5.0;
    const auto res = PidSim::simulateStepResponse(40.0, 0.0, 0.0, 210.0, 24.0, opt);
    int wechsel = 0;
    const int start = res.actualC.size() / 2;
    for (int i = start + 1; i < res.actualC.size(); ++i) {
        const double a = res.actualC[i - 1] - 210.0, b = res.actualC[i] - 210.0;
        if ((a < 0) != (b < 0))
            ++wechsel;
    }
    QVERIFY(wechsel >= 4);
}

void TestPrintFarm::pidSimWindupBounded()
{

    const auto mitAW = PidSim::simulateStepResponse(5.0, 5.0, 0.0, 210.0, 24.0);
    PidSim::Options optOhneAW;
    optOhneAW.antiWindup = false;
    const auto ohneAW = PidSim::simulateStepResponse(5.0, 5.0, 0.0, 210.0, 24.0, optOhneAW);
    QVERIFY(mitAW.overshootK < 30.0);
    QVERIFY(mitAW.overshootK < ohneAW.overshootK);
}

void TestPrintFarm::pidSimTmaxPhysicallyCapped()
{

    PidSim::Options opt;
    opt.tmaxC = 300.0;
    opt.durationS = 1200.0;
    const auto res = PidSim::simulateStepResponse(1000.0, 0.0, 0.0, 299.0, 24.0, opt);
    double tmax = 24.0;
    for (double v : res.actualC) tmax = qMax(tmax, v);
    QVERIFY(tmax < 305.0);
    QVERIFY(tmax > 290.0);
}

void TestPrintFarm::pidSimOutputQuantized()
{

    const auto res = PidSim::simulateStepResponse(5.0, 0.1, 2.0, 210.0, 24.0);
    for (double u : res.outputPercent)
        QVERIFY(qAbs(u - std::round(u)) < 1e-9);
}

void TestPrintFarm::pidSimFitFromHeatUp()
{

    PidSim::Options truth;
    truth.deadTimeS = 4.0;
    truth.blockCapacityJK = 8.0;
    truth.durationS = 250.0;
    const auto data = PidSim::simulateStepResponse(100000.0, 0.0, 0.0, 210.0, 24.0, truth);

    QVector<Sample> auf;
    for (int i = 0; i < data.timeS.size() && data.actualC[i] < 205.0; ++i)
        auf.append({data.timeS[i], data.actualC[i], 0, 0, 0, 0, 0, 0});
    QVERIFY(auf.size() > 30);
    const auto fit = PidSim::fitPlantFromHeatUp(auf, 210.0);
    QVERIFY(fit.sse >= 0.0);
    QVERIFY(qAbs(fit.opt.deadTimeS - 4.0) <= 1.0);

    double kLin, kRad;

    Q_UNUSED(kLin); Q_UNUSED(kRad);
    QVERIFY(fit.opt.blockCapacityJK > 0.4 * truth.blockCapacityJK);
    QVERIFY(fit.opt.blockCapacityJK < 2.5 * truth.blockCapacityJK);
}

void TestPrintFarm::updatePrinterInExport()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    Printer d = store.printer()[1];
    d.pidKp = 9.9;
    QVERIFY(store.updatePrinter(d));
    QCOMPARE(store.printer(store.printer()[1].printerId)->pidKp, 9.9);

    Printer fremd; fremd.printerId = "D999";
    QVERIFY(!store.updatePrinter(fremd));
}

void TestPrintFarm::invalidLineFormats()
{

    QVERIFY(!CsvIo::parseDate("32.13.2026").isValid());
    QVERIFY(!CsvIo::parseDateTime("2026-01-01").isValid());
    QVERIFY(CsvIo::parseDate("2026-01-01").isValid());
    QVERIFY(CsvIo::parseDateTime("2026-01-01 08:00:00").isValid());
}

void TestPrintFarm::qualityOvershoot()
{

    QVector<Sample> auf, dr;
    QVector<double> z, ist;
    for (int i = 0; i <= 60; ++i) {
        Sample m; m.timeS = i;
        m.nozzleActualC = (i < 40) ? 24.0 + (216.0 - 24.0) * i / 40.0
                               : 210.0 + 6.0 * std::exp(-(i - 40) / 10.0);
        auf.append(m); z.append(i); ist.append(m.nozzleActualC);
    }
    const ControlQuality rg = ControlQualityCalculator::computeValues(auf, dr, 210.0);
    QVERIFY(rg.available);
    QVERIFY(qAbs(rg.overshootK - 6.0) < 0.5);
    QVERIFY(rg.overshootPercent > 3.0);
    QVERIFY(rg.settled);
}

void TestPrintFarm::deviationOnlyPauses()
{

    QVector<Sample> printPhase;
    for (int i = 0; i < 5; ++i) {
        Sample m; m.timeS = 100.0 + i * 30; m.layer = 0; m.nozzleActualC = 150.0;
        printPhase.append(m);
    }
    double mittel, maxi;
    ControlQualityCalculator::deviationDuringPrint(printPhase, 210.0, mittel, maxi);
    QCOMPARE(mittel, 0.0);
    QCOMPARE(maxi, 0.0);
}

void TestPrintFarm::utf8Roundtrip()
{
    const QString dir = QDir::tempPath() + "/pf_utf8";
    QDir().mkpath(dir);
    const QString path = dir + "/umlaute.csv";
    const QList<QStringList> rows = {
        {"spalte"},
        {QString::fromUtf8("Düse verstopft - äöü ß ÄÖÜ  €; Semikolon")},
        {QString::fromUtf8("Zeile mit Komma? nein, aber Bindestrich – ok")},
    };
    QString err;
    QVERIFY(CsvIo::writeCsv(path, rows, &err));
    bool ok = false;
    const auto back = CsvIo::readCsv(path, &ok);
    QVERIFY(ok);
    QCOMPARE(back.size(), 3);
    QCOMPARE(back[1][0], rows[1][0]);
    QCOMPARE(back[2][0], rows[2][0]);
}

void TestPrintFarm::protocolHeatUpOnly()
{
    const QString dir = writeTestDatabase();

    QJsonObject root;
    QJsonObject job;
    job["auftrag_id"] = "JY"; job["drucker_id"] = "D001"; job["material_id"] = "MAT01";
    job["duesentemp_soll_c"] = 210.0; job["betttemp_soll_c"] = 60.0; job["schichtanzahl"] = 0;
    root["auftrag"] = job;
    QJsonObject auf;
    auf["abtastung_s"] = 1.0;
    QJsonArray z, di;
    for (int i = 0; i <= 50; ++i) { z.append(i); di.append(24.0 + 186.0 * i / 50.0); }
    auf["zeit_s"] = z; auf["duese_ist_c"] = di;
    root["aufheizen"] = auf;
    QFile f(dir + "/prot/onlyauf.json");
    f.open(QIODevice::WriteOnly);
    f.write(QJsonDocument(root).toJson());
    f.close();
    CsvIo::writeCsv(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"JY","prot/onlyauf.json","51","",""},
    });
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const PrintLog p = loader.load("JY");
    QVERIFY(p.valid);
    QCOMPARE(p.heatUp.size(), 51);
    QCOMPARE(p.printPhase.size(), 0);
    const ControlQuality rg = ControlQualityCalculator::computeValues(p.heatUp, p.printPhase, 210.0);
    QVERIFY(rg.available);
    QCOMPARE(rg.meanDeviationK, 0.0);
}

void TestPrintFarm::pathTraversalRejected()
{
    const QString dir = writeTestDatabase();

    QFile outside(dir + "/evil.json");
    outside.open(QIODevice::WriteOnly);
    outside.write("{\"auftrag\":{\"drucker_id\":\"X\"},\"aufheizen\":{\"zeit_s\":[0,1],\"duese_ist_c\":[20,21]}}");
    outside.close();
    CsvIo::writeCsv(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"JEVIL","../evil.json","2","",""},
        {"JABS","C:/Windows/win.ini","1","",""},
    });
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const PrintLog p1 = loader.load("JEVIL");
    QVERIFY(!p1.valid);
    QVERIFY(p1.error.contains(QStringLiteral("ausserhalb")));
    const PrintLog p2 = loader.load("JABS");
    QVERIFY(!p2.valid);
    QVERIFY(p2.error.contains(QStringLiteral("ausserhalb")));
}

void TestPrintFarm::csvFormulaInjectionBlocked()
{
    const QString dir = QDir::tempPath() + "/printfarm_csv_injection";
    QDir().mkpath(dir);
    const QString path = dir + "/out.csv";
    QVERIFY(CsvIo::writeCsv(path, {
        {"feld","wert"},
        {"=1+1", "@cmd", "+SUM(A1)", "-2+3", "normal", "mit,komma"},
    }));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString inhalt = QString::fromUtf8(f.readAll());
    f.close();

    QVERIFY(inhalt.contains(QStringLiteral("'=1+1")));
    QVERIFY(inhalt.contains(QStringLiteral("'@cmd")));
    QVERIFY(inhalt.contains(QStringLiteral("'+SUM(A1)")));
    QVERIFY(inhalt.contains(QStringLiteral("'-2+3")));
    QVERIFY(inhalt.contains(QStringLiteral("normal")));
    QVERIFY(inhalt.contains(QStringLiteral("\"mit,komma\"")));
}

void TestPrintFarm::weeklyConsumptionPerMaterial()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));

    const double v = store.meanWeeklyConsumptionKg(QStringLiteral("MAT01"));
    QVERIFY(qAbs(v - 0.21) < 0.01);

    QCOMPARE(store.meanWeeklyConsumptionKg(QStringLiteral("UNKNOWN")), 0.0);
}

static double ussMedian(const PidSim::Result &res, double anteil = 0.3)
{
    const int n = res.outputPercent.size();
    const int von = int(n * (1.0 - anteil));
    QVector<double> tail = res.outputPercent.mid(von);
    std::sort(tail.begin(), tail.end());
    return tail.isEmpty() ? -1.0 : tail[tail.size() / 2];
}

void TestPrintFarm::pidRankingSorted()
{

    const auto kand = PidSim::varyParametersAround(9.0, 0.6, 22.0);
    QCOMPARE(kand.size(), 7);
    QCOMPARE(kand.first(), QVector<double>({9.0, 0.6, 22.0}));
    PidSim::Options opt = PidSim::plantPresetForModel(QStringLiteral("MD01"));
    const auto ranking = PidSim::rankParameterCandidates(kand, 210.0, 24.0, opt);
    QCOMPARE(ranking.size(), kand.size());

    for (int i = 1; i < ranking.size(); ++i)
        QVERIFY(ranking[i - 1].score <= ranking[i].score);

    for (const auto &e : ranking) {
        QVERIFY(e.res.timeS.size() > 100);
        QVERIFY(qAbs(e.res.actualC.last() - 210.0) < 10.0);
    }

    int rankBase = -1, rankKiHigh = -1;
    for (int i = 0; i < ranking.size(); ++i) {
        if (ranking[i].kp == 9.0 && ranking[i].ki == 0.6 && ranking[i].kd == 22.0) rankBase = i;
        if (ranking[i].kp == 9.0 && ranking[i].ki == 1.2 && ranking[i].kd == 22.0) rankKiHigh = i;
    }
    QVERIFY(rankBase >= 0 && rankKiHigh >= 0);
    QVERIFY(rankKiHigh > rankBase);
}

void TestPrintFarm::pidSimUssInRealRange()
{

    const QStringList models = {"MD01","MD02","MD03","MD04","MD05","MD06","MD07","MD08"};
    for (const auto &mid : models) {
        PidSim::Options opt = PidSim::plantPresetForModel(mid);
        opt.durationS = 900.0;

        const auto res = PidSim::simulateStepResponse(5.0, 0.1, 2.0, 210.0, 24.0, opt);
        const double uss = ussMedian(res);
        QVERIFY2(uss >= 35.0 && uss <= 52.0,
                 qPrintable(QStringLiteral("Modell %1: u_ss(210)=%2 %").arg(mid).arg(uss)));
    }

    PidSim::Options optUnbekannt = PidSim::plantPresetForModel(QStringLiteral("MD11"));
    QCOMPARE(optUnbekannt.tmaxC, 400.0);
    optUnbekannt.durationS = 900.0;
    const auto ergUnbekannt = PidSim::simulateStepResponse(5.0, 0.1, 2.0, 210.0, 24.0, optUnbekannt);
    const double ussUnbekannt = ussMedian(ergUnbekannt);
    QVERIFY2(ussUnbekannt >= 35.0 && ussUnbekannt <= 52.0,
             qPrintable(QStringLiteral("Modell MD11 (unbekannt): u_ss(210)=%1 %")
                            .arg(ussUnbekannt)));

    QString dataDir = QCoreApplication::applicationDirPath() + "/../QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten";
    if (!QFileInfo::exists(dataDir + "/drucker.csv"))
        dataDir = QStringLiteral("QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten");
    QVERIFY2(QFileInfo::exists(dataDir + "/drucker.csv"), "echte Daten nicht gefunden");
    DataStore store;
    QVERIFY(store.loadFromFolder(dataDir));
    ProtocolLoader loader;
    QVERIFY(loader.loadIndex(dataDir) > 1600);
    int checked = 0;
    for (int i = 1; i <= 10; ++i) {
        const QString id = QStringLiteral("J%1").arg(i, 5, 10, QLatin1Char('0'));
        const PrintLog p = loader.load(id);

        if (p.heatUp.size() < 20 && p.printPhase.size() < 20) continue;
        const Printer *d = store.printer(p.printerId);
        QVERIFY(d);
        PidSim::Options opt = PidSim::plantPresetForModel(d->modelId);
        opt.durationS = 900.0;
        double measured;
        if (p.heatUp.size() >= 20) {

            QVector<double> tail;
            for (int k = int(p.heatUp.size() * 0.8); k < p.heatUp.size(); ++k)
                tail.append(p.heatUp[k].heaterPercent);
            std::sort(tail.begin(), tail.end());
            measured = tail[tail.size() / 2];
        } else {

            opt.printPhaseDisturbance = true;
            opt.printPhaseFromS = 0.0;
            QVector<double> tail;
            for (int k = int(p.printPhase.size() * 0.8); k < p.printPhase.size(); ++k)
                tail.append(p.printPhase[k].heaterPercent);
            std::sort(tail.begin(), tail.end());
            measured = tail[tail.size() / 2];
        }

        const auto res = PidSim::simulateStepResponse(5.0, 0.1, 2.0, p.nozzleTargetC, 24.0, opt);
        const double simUss = ussMedian(res);
        QVERIFY2(qAbs(simUss - measured) <= 8.0,
                 qPrintable(QStringLiteral("%1 (%2, %3 °C): sim u_ss=%4 %, gemessen=%5 %")
                                .arg(id, d->printerId).arg(p.nozzleTargetC).arg(simUss).arg(measured)));
        ++checked;
    }
    QCOMPARE(checked, 10);
}

void TestPrintFarm::plausibilityExtended()
{

    const QString dir = QDir::tempPath() + "/pf_plausibel";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","T","T-1","kartesisch","100","100","100","260","100","40","100"},
        {"MD06","T","P-Mini","kartesisch","180","180","180","2500","80","30","120"},
    });
    CsvIo::writeCsv(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20","100"},
    });

    QFile fCsv(dir + "/drucker.csv");
    QVERIFY(fCsv.open(QIODevice::WriteOnly | QIODevice::Text));
    fCsv.write("drucker_id,modell_id,standort,inbetriebnahme,betriebsstunden,duese_mm,"
               "firmware,pid_kp,pid_ki,pid_kd,status\r\n"
               "D048,MD01,A-1,2024-01-01,100,0.4,1.0,5,0.1,2,bereit\r\n"
               "D048,MD01,B-1,2024-01-01,100,0.4,1.0,5,0.1,2,bereit\r\n"
               "D014,MD01,A-2,2024-01-01,7694,4,2.2.0,8.09,0.616,22.6,bereit\r\n"
               "D016,MD06,B-3,2025-02-25,5143,0.8,2.2.0,-9.26,0.576,26.3,bereit\r\n"
               "D020,MD01,A-4,2024-01-01,-5,0.4,1.0,5,0.1,2,bereit\r\n"
               "D021,MD01,A-5,2024-01-01,abc,0.4,1.0,5,0.1,2,bereit\r\n");
    fCsv.close();
    CsvIo::writeCsv(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min",
         "filament_soll_g","ergebnis","fehlercode","bemerkung"},
        {"J00001","D048","MAT01","Teil1","2026-01-01 08:00:00","2026-01-01 09:00:00","60","30","erfolgreich","",""},
    });
    CsvIo::writeCsv(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.printer().size(), 6);
    auto enthaelt = [&](const QString &teil) {
        for (const auto &e : store.log())
            if (e.category == LogEntry::Validation && e.message.contains(teil))
                return true;
        return false;
    };
    QVERIFY(enthaelt(QStringLiteral("Duplikat-ID „D048“")));
    QVERIFY(enthaelt(QStringLiteral("2500")));
    QVERIFY(enthaelt(QStringLiteral("negativer PID-Wert")));
    QVERIFY(enthaelt(QStringLiteral("Düse 4 mm")));
    QVERIFY(enthaelt(QStringLiteral("negative Betriebsstunden")));
    QVERIFY(enthaelt(QStringLiteral("„abc“ ist keine Zahl")));
}

void TestPrintFarm::sqliteImportMasterDataRoundtrip()
{
    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    const QString dbPath = QDir::tempPath() + "/printfarm_stammdaten.sqlite";
    QFile::remove(dbPath);
    QString err;
    QVERIFY(store.exportSqlite(dbPath, &err));

    DataStore store2;
    QVERIFY(store2.importSqlite(dbPath));
    QCOMPARE(store2.printer().size(), 2);
    QCOMPARE(store2.auftraege().size(), 3);
    QCOMPARE(store2.models().size(), 1);
    QCOMPARE(store2.materialien().size(), 1);
    QCOMPARE(store2.maintenances().size(), 1);
    QVERIFY(store2.model(QStringLiteral("MD01")) != nullptr);
    QVERIFY(store2.material(QStringLiteral("MAT01")) != nullptr);
    QVERIFY(store2.printer(QStringLiteral("D001")) != nullptr);
    QCOMPARE(store2.model(QStringLiteral("MD01"))->maxNozzleTempC, 260.0);
    QCOMPARE(store2.maintenances().first().maintenanceId, QStringLiteral("W0001"));

    bool md99 = false;
    for (const auto &e : store2.log())
        if (e.category == LogEntry::Validation && e.message.contains(QStringLiteral("MD99")))
            md99 = true;
    QVERIFY(md99);

    QVERIFY(store2.importSqlite(dbPath));
    QCOMPARE(store2.printer().size(), 2);
    QVERIFY(store2.model(QStringLiteral("MD01")) != nullptr);
    QCOMPARE(store2.printer(QStringLiteral("D001"))->nozzleMm, 0.4);
}

void TestPrintFarm::sqliteExportDuplicates()
{
    const QString dir = QDir::tempPath() + "/pf_sqlite_duplikat";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","T","T-100","kartesisch","100","100","100","260","100","40","100"},
    });
    CsvIo::writeCsv(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20","100"},
    });
    CsvIo::writeCsv(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm",
         "firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-01-1","2024-01-01","100","0.4","1.0","5.0","0.1","2.0","bereit"},
        {"D002","MD01","A-01-2","2024-01-01","50","0.4","1.0","5.0","0.1","2.0","bereit"},
        {"D001","MD01","B-9-9","2024-02-02","999","0.8","2.0","9.0","0.9","9.0","defekt"},
    });
    CsvIo::writeCsv(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min",
         "filament_soll_g","ergebnis","fehlercode","bemerkung"},
        {"J00001","D001","MAT01","Teil1","2026-01-01 08:00:00","2026-01-01 09:00:00","60","30","erfolgreich","",""},
        {"J00001","D001","MAT01","Teil1-dup","","","120","50","ausstehend","",""},
    });
    CsvIo::writeCsv(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},
        {"W0001","D001","2026-01-03","reinigung","20","ok"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.printer().size(), 3);
    QCOMPARE(store.auftraege().size(), 2);

    QCOMPARE(store.printer(QStringLiteral("D001"))->location, QStringLiteral("B-9-9"));

    const QString dbPath = QDir::tempPath() + "/printfarm_duplikat.sqlite";
    QFile::remove(dbPath);
    QString err;
    QVERIFY2(store.exportSqlite(dbPath, &err), qPrintable(err));

    bool exportDupLog = false;
    for (const auto &e : store.log())
        if (e.message.contains(QStringLiteral("SQLite-Export")) &&
            e.message.contains(QStringLiteral("Duplikat-ID")) &&
            e.message.contains(QStringLiteral("D001")))
            exportDupLog = true;
    QVERIFY(exportDupLog);

    DataStore store2;
    QVERIFY(store2.importSqlite(dbPath));
    QCOMPARE(store2.printer().size(), 2);
    QCOMPARE(store2.auftraege().size(), 1);
    QCOMPARE(store2.printer(QStringLiteral("D001"))->location, QStringLiteral("B-9-9"));
    QCOMPARE(store2.printer(QStringLiteral("D001"))->operatingHours, 999.0);
    QCOMPARE(store2.auftraege().first().part, QStringLiteral("Teil1-dup"));

    const QString altDbPath = QDir::tempPath() + "/printfarm_alte_duplikat_db.sqlite";
    QFile::remove(altDbPath);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("pf_alte_db"));
        db.setDatabaseName(altDbPath);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(q.exec("CREATE TABLE drucker (drucker_id TEXT, modell_id TEXT, standort TEXT, inbetriebnahme TEXT,"
                       " betriebsstunden REAL, duese_mm REAL, firmware TEXT, pid_kp REAL, pid_ki REAL, pid_kd REAL, status TEXT)"));
        QVERIFY(q.exec("CREATE TABLE auftraege (auftrag_id TEXT, drucker_id TEXT, material_id TEXT, bauteil TEXT,"
                       " start TEXT, ende TEXT, dauer_soll_min REAL, filament_soll_g REAL, ergebnis TEXT, fehlercode TEXT, bemerkung TEXT)"));
        QVERIFY(q.exec("INSERT INTO drucker VALUES ('D001','MD01','alt','2024-01-01',1,0.4,'1',1,1,1,'bereit')"));
        QVERIFY(q.exec("INSERT INTO drucker VALUES ('D001','MD01','neu','2024-01-01',2,0.4,'1',1,1,1,'defekt')"));
        QVERIFY(q.exec("INSERT INTO auftraege VALUES ('J1','D001','MAT01','t','','',1,1,'erfolgreich','','')"));
        q = QSqlQuery();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("pf_alte_db"));
    }
    DataStore store3;
    QVERIFY(store3.importSqlite(altDbPath));
    QCOMPARE(store3.printer().size(), 1);
    QCOMPARE(store3.printer(QStringLiteral("D001"))->location, QStringLiteral("neu"));
    bool importDupLog = false;
    for (const auto &e : store3.log())
        if (e.message.contains(QStringLiteral("SQLite-Import")) &&
            e.message.contains(QStringLiteral("Duplikat-ID")) &&
            e.message.contains(QStringLiteral("D001")))
            importDupLog = true;
    QVERIFY(importDupLog);
}

void TestPrintFarm::planningFreeAfterLastJob()
{

    const QString dir = QDir::tempPath() + "/pf_planung_freiab";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","T","T-100","kartesisch","100","100","100","260","100","40","100"},
    });
    CsvIo::writeCsv(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20.0","100"},
    });
    CsvIo::writeCsv(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm",
         "firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-01-1","2024-01-01","100","0.4","1.0","5.0","0.1","2.0","bereit"},
    });
    CsvIo::writeCsv(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min",
         "filament_soll_g","ergebnis","fehlercode","bemerkung"},

        {"J00001","D001","MAT01","Teil1","2026-01-06 06:00:00","2026-01-06 10:00:00","240","30","erfolgreich","",""},
        {"J00002","D001","MAT01","Teil2","","","60","10","ausstehend","",""},
    });
    CsvIo::writeCsv(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},

        {"W0001","D001","2026-01-07","reinigung","60","ok"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QList<Job> pending;
    for (const auto &a : store.auftraege())
        if (a.jobResult == QLatin1String("ausstehend")) pending.append(a);
    const QDateTime startTime(QDate(2026, 1, 5), QTime(8, 0));
    const auto res = Planning::assignJobsGreedy(pending, store.printer(), store, startTime);
    QCOMPARE(res.size(), 1);

    QCOMPARE(res[0].von, QDateTime(QDate(2026, 1, 7), QTime(1, 0)));
    QCOMPARE(res[0].bis, res[0].von.addSecs(3600));
}

void TestPrintFarm::errorDetectionHeaterFailed()
{

    PrintLog p;
    p.jobId = "JX"; p.nozzleTargetC = 210.0; p.bedTargetC = 60.0;
    for (int i = 0; i < 60; ++i) {
        Sample m; m.timeS = i; m.nozzleActualC = 24.0 + 0.01 * i;
        m.heaterPercent = 100.0;
        p.heatUp.append(m);
    }
    ControlQuality rg;
    const auto b1 = ErrorDetection::detectFaults(p, rg, nullptr);
    bool heater = false;
    for (const auto &f : b1.errorPatterns)
        if (f.contains(QStringLiteral("Heizung"))) heater = true;
    QVERIFY(heater);

    for (int i = 0; i < 60; ++i)
        p.heatUp[i].nozzleActualC = 24.0 + 3.0 * i;
    const auto b2 = ErrorDetection::detectFaults(p, rg, nullptr);
    for (const auto &f : b2.errorPatterns)
        QVERIFY(!f.contains(QStringLiteral("Heizung")));
}

void TestPrintFarm::errorDetectionBedTempFormat()
{

    PrintLog p;
    p.jobId = "JX"; p.nozzleTargetC = 210.0; p.bedTargetC = 60.0;
    ControlQuality rg;
    Material mat;
    mat.materialId = "MAT1"; mat.description = "PLA";
    mat.nozzleTempC = 210.0; mat.bedTempC = 45.0;
    const auto b = ErrorDetection::detectFaults(p, rg, &mat);
    QString bedMessage;
    for (const auto &f : b.errorPatterns)
        if (f.contains(QStringLiteral("Betttemperatur"))) bedMessage = f;
    QVERIFY(!bedMessage.isEmpty());
    QVERIFY(!bedMessage.contains(QStringLiteral("%.0f")));
    QVERIFY(bedMessage.contains(QStringLiteral("60")));
    QVERIFY(bedMessage.contains(QStringLiteral("45")));
}

void TestPrintFarm::logCappedWithoutConsecutiveDuplicates()
{

    DataStore store;
    store.log(LogEntry::PrintLog, QStringLiteral("Auftrag J1: Fehler X"), QStringLiteral("J1"));
    const int n = store.log().size();
    store.log(LogEntry::PrintLog, QStringLiteral("Auftrag J1: Fehler X"), QStringLiteral("J1"));
    QCOMPARE(store.log().size(), n);

    store.log(LogEntry::PrintLog, QStringLiteral("Auftrag J1: Fehler Y"), QStringLiteral("J1"));
    QCOMPARE(store.log().size(), n + 1);
    store.log(LogEntry::PrintLog, QStringLiteral("Auftrag J1: Fehler X"), QStringLiteral("J2"));
    QCOMPARE(store.log().size(), n + 2);

    store.log(LogEntry::System, QStringLiteral("Zwischeneintrag"));
    store.log(LogEntry::PrintLog, QStringLiteral("Auftrag J1: Fehler X"), QStringLiteral("J1"));
    QCOMPARE(store.log().size(), n + 4);

    DataStore store2;
    const int gesamt = DataStore::kMaxLogEntries + 250;
    for (int i = 0; i < gesamt; ++i)
        store2.log(LogEntry::System, QStringLiteral("Meldung %1").arg(i));
    QCOMPARE(store2.log().size(), DataStore::kMaxLogEntries);
    QCOMPARE(store2.log().first().message, QStringLiteral("Meldung %1").arg(gesamt - DataStore::kMaxLogEntries));
    QCOMPARE(store2.log().last().message, QStringLiteral("Meldung %1").arg(gesamt - 1));
}

void TestPrintFarm::logModelCachedIndexAboveCap()
{

    DataStore store;
    LogModel model(&store);
    QCOMPARE(model.rowCount(), 0);
    const int gesamt = DataStore::kMaxLogEntries + 100;
    for (int i = 0; i < gesamt; ++i)
        store.log(i % 2 == 0 ? LogEntry::PrintLog : LogEntry::System,
                     QStringLiteral("M%1").arg(i), QStringLiteral("J%1").arg(i % 3));
    QCOMPARE(store.log().size(), DataStore::kMaxLogEntries);
    model.refresh();
    QCOMPARE(model.rowCount(), DataStore::kMaxLogEntries);

    QCOMPARE(model.data(model.index(0, LogModel::ColMessage), Qt::DisplayRole).toString(),
             QStringLiteral("M%1").arg(gesamt - 1));
    QCOMPARE(model.data(model.index(0, LogModel::ColJob), Qt::DisplayRole).toString(),
             QStringLiteral("J%1").arg((gesamt - 1) % 3));

    QCOMPARE(model.data(model.index(model.rowCount() - 1, LogModel::ColMessage), Qt::DisplayRole).toString(),
             QStringLiteral("M%1").arg(gesamt - DataStore::kMaxLogEntries));

    model.setCategoryFilter(LogEntry::PrintLog);
    int erwartet = 0, letzterProt = -1;
    for (int i = gesamt - DataStore::kMaxLogEntries; i < gesamt; ++i)
        if (i % 2 == 0) { ++erwartet; letzterProt = i; }
    QCOMPARE(model.rowCount(), erwartet);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage), Qt::DisplayRole).toString(),
             QStringLiteral("M%1").arg(letzterProt));
    QCOMPARE(model.data(model.index(0, LogModel::ColCategory), Qt::DisplayRole).toString(), QStringLiteral("Protokoll"));
    model.setCategoryFilter(-1);
    QCOMPARE(model.rowCount(), DataStore::kMaxLogEntries);

    connect(&store, &DataStore::logChanged, &model, &LogModel::refresh);
    store.log(LogEntry::System, QStringLiteral("nachcap"), QStringLiteral("JX"));
    QCOMPARE(model.rowCount(), DataStore::kMaxLogEntries);
    QCOMPARE(model.data(model.index(0, LogModel::ColMessage), Qt::DisplayRole).toString(), QStringLiteral("nachcap"));

    QVERIFY(!model.data(model.index(model.rowCount(), LogModel::ColMessage), Qt::DisplayRole).isValid());
}

void TestPrintFarm::materialSortWithManyMaterials()
{

    const QString dir = QDir::tempPath() + "/pf_n7_materialsort";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","Test","T-100","kartesisch","300","300","300","260","100","40","100"},
    });
    const int anzahlMat = 300;
    QList<QStringList> matRows = {{"material_id","bezeichnung","duesentemp_c","betttemp_c",
                                    "dichte_g_cm3","preis_eur_kg","luefter_prozent"}};
    for (int i = 0; i < anzahlMat; ++i)
        matRows.append({QStringLiteral("MAT%1").arg(i, 3, 10, QLatin1Char('0')),
                        QStringLiteral("Material %1").arg(i), "210","60","1.24","20","100"});
    CsvIo::writeCsv(dir + "/materialien.csv", matRows);
    CsvIo::writeCsv(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm",
         "firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-01-1","2024-01-01","100","0.4","1.0","5.0","0.1","2.0","bereit"},
    });
    const int anzahlAuf = 2000;
    QList<QStringList> aufRows = {{"auftrag_id","drucker_id","material_id","bauteil","start","ende",
                                    "dauer_soll_min","filament_soll_g","ergebnis","fehlercode","bemerkung"}};
    const QDateTime basis(QDate(2026, 1, 1), QTime(0, 0));
    for (int i = 0; i < anzahlAuf; ++i) {
        const QDateTime start = basis.addSecs(qint64(i) * 600);
        aufRows.append({QStringLiteral("J%1").arg(i, 5, 10, QLatin1Char('0')), "D001",
                        QStringLiteral("MAT%1").arg(i % anzahlMat, 3, 10, QLatin1Char('0')),
                        "Teil", CsvIo::formatDateTime(start), CsvIo::formatDateTime(start.addSecs(3600)),
                        "60", QString::number(10 + (i % 50)), "erfolgreich", "", ""});
    }
    CsvIo::writeCsv(dir + "/auftraege.csv", aufRows);
    CsvIo::writeCsv(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    MaterialModel model(&store);
    QCOMPARE(model.rowCount(), anzahlMat);

    model.sort(MaterialModel::ColWeeklyConsumption, Qt::AscendingOrder);
    for (int i = 1; i < model.rowCount(); ++i)
        QVERIFY(model.data(model.index(i - 1, MaterialModel::ColWeeklyConsumption), Qt::DisplayRole).toDouble()
                <= model.data(model.index(i, MaterialModel::ColWeeklyConsumption), Qt::DisplayRole).toDouble());

    model.sort(MaterialModel::ColWeeklyConsumption, Qt::DescendingOrder);
    for (int i = 1; i < model.rowCount(); ++i)
        QVERIFY(model.data(model.index(i - 1, MaterialModel::ColWeeklyConsumption), Qt::DisplayRole).toDouble()
                >= model.data(model.index(i, MaterialModel::ColWeeklyConsumption), Qt::DisplayRole).toDouble());

    for (int i = 0; i < model.rowCount(); i += 37) {
        const QString id = model.data(model.index(i, MaterialModel::ColId), Qt::DisplayRole).toString();
        const double soll = qRound(store.meanWeeklyConsumptionKg(id) * 100.0) / 100.0;
        QCOMPARE(model.data(model.index(i, MaterialModel::ColWeeklyConsumption), Qt::DisplayRole).toDouble(), soll);
    }

    model.sort(MaterialModel::ColBez, Qt::AscendingOrder);
    for (int i = 1; i < model.rowCount(); ++i)
        QVERIFY(model.data(model.index(i - 1, MaterialModel::ColBez), Qt::DisplayRole).toString()
                <= model.data(model.index(i, MaterialModel::ColBez), Qt::DisplayRole).toString());
    model.sort(MaterialModel::ColId, Qt::AscendingOrder);
}

void TestPrintFarm::invalidDateIsLogged()
{

    const QString dir = QDir::tempPath() + "/pf_datum_ungueltig";
    QDir().mkpath(dir);
    CsvIo::writeCsv(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","T","T-1","kartesisch","100","100","100","260","100","40","100"},
    });
    CsvIo::writeCsv(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20","100"},
    });
    CsvIo::writeCsv(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm",
         "firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-1","2025-02-31","100","0.4","1.0","5","0.1","2","bereit"},
    });
    CsvIo::writeCsv(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min",
         "filament_soll_g","ergebnis","fehlercode","bemerkung"},
    });
    CsvIo::writeCsv(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},
        {"W0001","D001","2025-02-31","reinigung","20","ok"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.printer().size(), 1);
    QCOMPARE(store.maintenances().size(), 1);
    QVERIFY(!store.printer(QStringLiteral("D001"))->commissionedOn.isValid());
    QVERIFY(!store.maintenances().first().date.isValid());
    bool printerLogged = false, maintenanceLogged = false;
    for (const auto &e : store.log()) {
        if (e.category != LogEntry::Validation) continue;
        if (!e.message.contains(QString::fromUtf8("ungültiges Datum"))) continue;
        if (e.message.contains(QStringLiteral("drucker.csv")) && e.message.contains(QStringLiteral("2025-02-31")))
            printerLogged = true;
        if (e.message.contains(QStringLiteral("wartungen.csv")) && e.message.contains(QStringLiteral("2025-02-31")))
            maintenanceLogged = true;
    }
    QVERIFY(printerLogged);
    QVERIFY(maintenanceLogged);
}

void TestPrintFarm::sqliteImportWithoutRequiredTables()
{

    const QString dir = writeTestDatabase();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.printer().size(), 2);
    QCOMPARE(store.auftraege().size(), 3);

    const QString badDb = QDir::tempPath() + "/printfarm_p3_bad.sqlite";
    QFile::remove(badDb);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("p3_bad"));
        db.setDatabaseName(badDb);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE modelle (modell_id TEXT PRIMARY KEY)")));
        q = QSqlQuery();
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(QStringLiteral("p3_bad"));
    }
    const int logBefore = store.log().size();
    QVERIFY(!store.importSqlite(badDb));
    bool rejectedLogged = false;
    for (int i = logBefore; i < store.log().size(); ++i)
        if (store.log()[i].category == LogEntry::File &&
            store.log()[i].message.contains(QStringLiteral("Pflicht-Tabelle")))
            rejectedLogged = true;
    QVERIFY(rejectedLogged);

    QCOMPARE(store.printer().size(), 2);
    QCOMPARE(store.auftraege().size(), 3);
    QCOMPARE(store.models().size(), 1);
    QVERIFY(store.printer(QStringLiteral("D001")) != nullptr);
    QCOMPARE(store.printer(QStringLiteral("D001"))->location, QStringLiteral("A-01-1"));

    const QString leerDb = QDir::tempPath() + "/printfarm_p3_leer.sqlite";
    QFile::remove(leerDb);
    QFile f(leerDb);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();
    QVERIFY(!store.importSqlite(leerDb));
    QCOMPARE(store.printer().size(), 2);
    QCOMPARE(store.auftraege().size(), 3);
}

QTEST_GUILESS_MAIN(TestPrintFarm)
#include "tst_printfarm.moc"
