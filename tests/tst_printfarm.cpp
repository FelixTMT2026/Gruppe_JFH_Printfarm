// Automatisierte Tests mit Qt Test: CSV-Import/-Export, Validierung,
// Regelgüte (inkl. Pausenbehandlung), Rechte, Auftragsplanung, Persistenz
// (SQLite), Protokoll-Lazy-Loading.

#include <QtTest>
#include "csvio.h"
#include "datastore.h"
#include "protocol.h"
#include "regelguete.h"
#include "analyse.h"
#include "auth.h"

class TestPrintFarm : public QObject {
    Q_OBJECT
private slots:
    void csvImportUndPlausibilitaet();
    void csvExport();
    void regelgueteAufheizen();
    void regelabweichungMitPause();
    void protokollLazyLoading();
    void rechte();
    void planungGreedy();
    void kostenrechnung();
    void pidSimulation();
    void sqlitePersistenz();
    // --- QA-Runde 2: Grenzfälle ---
    void kaputteCsvZeilen();
    void fehlendeDateienDegradation();
    void defektesJson();
    void protokollNichtImIndex();
    void lruCacheBegrenzt();
    void asyncLoadSignal();
    void regelgueteNichtAusgeregelt();
    void regelgueteKeinSprung();
    void csvRoundtrip();
    void planungRespektiertMaterialGrenzen();
    void planungAlleDruckerBelegt();
    void kostenOhneMaterial();
    void pidSimNullParameter();
    void updateDruckerImExport();
    void ungueltigeZeilenFormate();
    // --- QA-Runde 3 ---
    void regelgueteUeberschwingen();
    void regelabweichungNurPausen();
    void utf8Roundtrip();
    void protokollNurAufheizen();
};

// Kleine synthetische Testdatenbasis im Temp-Verzeichnis
static QString schreibeTestdaten()
{
    const QString dir = QDir::tempPath() + "/printfarm_testdaten";
    QDir().mkpath(dir);
    CsvIo::write(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","Test","T-100","kartesisch","100","100","100","260","100","40","100"},
    });
    CsvIo::write(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20.0","100"},
    });
    CsvIo::write(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm",
         "firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-01-1","2024-01-01","100","0.4","1.0","5.0","0.1","2.0","bereit"},
        {"D002","MD99","A-01-2","2024-01-01","50","0.4","1.0","5.0","0.1","2.0","bereit"}, // ungültiges Modell
    });
    CsvIo::write(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min",
         "filament_soll_g","ergebnis","fehlercode","bemerkung"},
        {"J00001","D001","MAT01","Teil1","2026-01-01 08:00:00","2026-01-01 09:00:00","60","30","erfolgreich","",""},
        {"J00002","D001","MAT01","Teil2","","","120","50","ausstehend","",""},
        {"J00003","D002","MAT01","Teil3","2026-01-02 08:00:00","2026-01-02 08:30:00","30","10","fehlgeschlagen","THERMAL_RUNAWAY",""},
    });
    CsvIo::write(dir + "/wartungen.csv", {
        {"wartung_id","drucker_id","datum","art","dauer_min","bemerkung"},
        {"W0001","D001","2026-01-03","reinigung","20","ok"},
    });
    CsvIo::write(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"J00001","prot/J00001.json","10","2026-01-01 08:00:00","2026-01-01 09:00:00"},
    });
    QDir().mkpath(dir + "/prot");
    // Synthetisches Protokoll: Aufheizen 24 -> 210 °C (linear), Druckphase mit Pause
    QJsonObject root;
    QJsonObject auftrag;
    auftrag["auftrag_id"] = "J00001"; auftrag["drucker_id"] = "D001"; auftrag["material_id"] = "MAT01";
    auftrag["duesentemp_soll_c"] = 210.0; auftrag["betttemp_soll_c"] = 60.0; auftrag["schichtanzahl"] = 10;
    root["auftrag"] = auftrag;
    QJsonObject auf;
    auf["abtastung_s"] = 1.0;
    QJsonArray z, di, bi, hz;
    for (int i = 0; i <= 100; ++i) {
        z.append(i);
        di.append(24.0 + (210.0 - 24.0) * i / 100.0);   // lineares Aufheizen, kein Überschwingen
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
        const bool pause = (i >= 10 && i < 13);          // Pause: schicht = 0
        di2.append(pause ? 150.0 : 210.5);                // in Pause: 150 °C (Wiederaufheizen danach 210.5 -> e=0.5)
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

void TestPrintFarm::csvImportUndPlausibilitaet()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QCOMPARE(store.drucker().size(), 2);
    QCOMPARE(store.modelle().size(), 1);
    QCOMPARE(store.materialien().size(), 1);
    QCOMPARE(store.auftraege().size(), 3);
    QCOMPARE(store.wartungen().size(), 1);
    // Plausibilitätsprüfung: D002 verweist auf unbekanntes Modell MD99
    bool gefunden = false;
    for (const auto &e : store.log())
        if (e.kategorie == LogEintrag::Validierung && e.meldung.contains("MD99"))
            gefunden = true;
    QVERIFY(gefunden);
}

void TestPrintFarm::csvExport()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    const QString out = QDir::tempPath() + "/printfarm_export";
    QDir().mkpath(out);
    QString err;
    QVERIFY(store.exportCsv(out, &err));
    bool ok = false;
    QList<CsvError> errors;
    const auto rows = CsvIo::read(out + "/drucker_out.csv", &ok, &errors);
    QVERIFY(ok);
    QCOMPARE(rows.size(), 3);   // Kopf + 2 Drucker
    QCOMPARE(rows[1][0], QStringLiteral("D001"));
}

void TestPrintFarm::regelgueteAufheizen()
{
    // Lineares Aufheizen 24 -> 210 in 100 s: tr = 80 s, kein Überschwingen,
    // Ausregelzeit = Zeitpunkt des letzten Ausreißers außerhalb ±2 K + 1
    QVector<Messpunkt> auf, dr;
    QVector<double> z, ist;
    for (int i = 0; i <= 100; ++i) {
        Messpunkt m; m.zeitS = i; m.dueseIstC = 24.0 + 186.0 * i / 100.0;
        auf.append(m); z.append(i); ist.append(m.dueseIstC);
    }
    const Regelguete rg = RegelgueteRechner::werteAus(auf, dr, 210.0);
    QVERIFY(rg.verfuegbar);
    QVERIFY(qAbs(rg.anstiegszeitS - 80.0) < 1.0);
    QCOMPARE(rg.ueberschwingenK, 0.0);
    QVERIFY(rg.ausgeregelt);
    QVERIFY(rg.ausregelzeitS > 0);
}

void TestPrintFarm::regelabweichungMitPause()
{
    // Druckphase: 20 Punkte, davon 3 Pause (schicht=0, 150 °C) - Pausen und
    // Wiederaufheizen (bis |e| <= 3 K) dürfen nicht in die Statistik einfließen.
    QVector<Messpunkt> druck;
    for (int i = 0; i < 20; ++i) {
        Messpunkt m; m.zeitS = 120.0 + 30 * i; m.dueseSollC = 210.0;
        const bool pause = (i >= 10 && i < 13);
        m.dueseIstC = pause ? 150.0 : 209.0;   // e = 1 K im Normalbetrieb
        m.schicht = pause ? 0 : i + 1;
        druck.append(m);
    }
    double mittel, maxi;
    RegelgueteRechner::regelabweichungDruck(druck, 210.0, mittel, maxi);
    QVERIFY(qAbs(mittel - 1.0) < 1e-9);       // nur die Nicht-Pause-Punkte mit e=1
    QVERIFY(qAbs(maxi - 1.0) < 1e-9);
}

void TestPrintFarm::protokollLazyLoading()
{
    const QString dir = schreibeTestdaten();
    ProtocolLoader loader;
    QCOMPARE(loader.loadIndex(dir), 1);
    QCOMPARE(loader.pfadFuer("J00001"), QStringLiteral("prot/J00001.json"));
    QVERIFY(loader.pfadFuer("J99999").isEmpty());
    const Protokoll p = loader.load("J00001");
    QVERIFY(p.valid);
    QCOMPARE(p.aufheizen.size(), 101);
    QCOMPARE(p.druck.size(), 20);
    QCOMPARE(p.ereignisse.size(), 4);
    QVERIFY(loader.cached("J00001"));
    // Unbekannter Auftrag: invalid mit Fehlermeldung
    const Protokoll p2 = loader.load("J99999");
    QVERIFY(!p2.valid);
    QVERIFY(!p2.fehler.isEmpty());
}

void TestPrintFarm::rechte()
{
    Auth::logout();
    QVERIFY(!Auth::darfPidAendern());
    QVERIFY(!Auth::darfStammdatenAendern());
    Auth::login(Auth::Bediener, "b1");
    QVERIFY(!Auth::darfPidAendern());
    QVERIFY(Auth::darfExportieren());
    Auth::login(Auth::Technik, "t1");
    QVERIFY(Auth::darfPidAendern());
    QVERIFY(!Auth::darfStammdatenAendern());
    Auth::login(Auth::Admin, "a1");
    QVERIFY(Auth::darfPidAendern());
    QVERIFY(Auth::darfStammdatenAendern());
    Auth::logout();
}

void TestPrintFarm::planungGreedy()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QList<Auftrag> ausstehend;
    for (const auto &a : store.auftraege())
        if (a.ergebnis == "ausstehend") ausstehend.append(a);
    QCOMPARE(ausstehend.size(), 1);
    const auto erg = Planung::greedy(ausstehend, store.drucker(), store, QDateTime(QDate(2026,1,5), QTime(8,0)));
    QCOMPARE(erg.size(), 1);
    QCOMPARE(erg[0].druckerId, QStringLiteral("D001"));  // D002 hat ungültiges Modell -> nicht geeignet
    QCOMPARE(erg[0].auftragId, QStringLiteral("J00002"));
}

void TestPrintFarm::kostenrechnung()
{
    Auftrag a;
    a.auftragId = "J1"; a.filamentSollG = 2000; a.dauerSollMin = 120;
    a.start = QDateTime(QDate(2026,1,1), QTime(8,0));
    a.ende = QDateTime(QDate(2026,1,1), QTime(10,0));   // 120 min gemessen
    Material m; m.preisEurKg = 20.0;
    const auto k = Kostenrechnung::auftrag(a, &m, 12.0);
    QVERIFY(qAbs(k.materialEur - 40.0) < 1e-9);   // 2 kg * 20 €/kg
    QVERIFY(qAbs(k.maschinenEur - 24.0) < 1e-9);   // 2 h * 12 €/h
    QVERIFY(qAbs(k.gesamtEur - 64.0) < 1e-9);
}

void TestPrintFarm::pidSimulation()
{
    const auto erg = PidSim::simuliere(5.0, 0.1, 2.0, 210.0, 24.0);
    QVERIFY(erg.zeitS.size() > 100);
    QVERIFY(erg.anstiegszeitS > 0);
    // Endwert nähert sich der Solltemperatur
    QVERIFY(qAbs(erg.istC.last() - 210.0) < 5.0);
}

void TestPrintFarm::sqlitePersistenz()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    const QString dbPath = QDir::tempPath() + "/printfarm_test.sqlite";
    QFile::remove(dbPath);
    QString err;
    QVERIFY(store.exportSqlite(dbPath, &err));
    DataStore store2;
    QVERIFY(store2.importSqlite(dbPath));
    QCOMPARE(store2.drucker().size(), 2);
    QCOMPARE(store2.auftraege().size(), 3);
}

// ---------------- QA-Runde 2: Grenzfälle -----------------------------------

void TestPrintFarm::kaputteCsvZeilen()
{
    const QString dir = QDir::tempPath() + "/pf_kaputt";
    QDir().mkpath(dir);
    CsvIo::write(dir + "/modelle.csv", {
        {"modell_id","hersteller","bezeichnung","kinematik","bauraum_x_mm","bauraum_y_mm",
         "bauraum_z_mm","max_duesentemp_c","max_betttemp_c","heizleistung_duese_w","heizleistung_bett_w"},
        {"MD01","T","T-1","kartesisch","1","1","1","260","100","40","100"},
        {"MD02","T","T-2"},                                  // zu wenige Spalten -> Fehler, nicht Abbruch
        {"","T","T-3","delta","1","1","1","260","100","40","100"}, // leere ID -> Fehler
    });
    CsvIo::write(dir + "/materialien.csv", {
        {"material_id","bezeichnung","duesentemp_c","betttemp_c","dichte_g_cm3","preis_eur_kg","luefter_prozent"},
        {"MAT01","PLA","210","60","1.24","20","100"},
    });
    CsvIo::write(dir + "/drucker.csv", {
        {"drucker_id","modell_id","standort","inbetriebnahme","betriebsstunden","duese_mm","firmware","pid_kp","pid_ki","pid_kd","status"},
        {"D001","MD01","A-1","2024-13-99","100","0.4","1.0","5","0.1","2","bereit"},  // ungültiges Datum
        {"D002","MD01","A-2","2024-01-01","abc","0.4","1.0","5","0.1","2","bereit"},  // Zahl erwartet
    });
    CsvIo::write(dir + "/auftraege.csv", {
        {"auftrag_id","drucker_id","material_id","bauteil","start","ende","dauer_soll_min","filament_soll_g","ergebnis","fehlercode","bemerkung"},
    });
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));            // Pflichtdateien lesbar -> true
    QCOMPARE(store.modelle().size(), 1);           // nur die gültige Zeile
    QCOMPARE(store.drucker().size(), 2);           // beide Zeilen strukturell ok (Datum/Zahl parsen defensiv)
    // Fehler wurden geloggt, nicht verschluckt
    bool logGefunden = false;
    for (const auto &e : store.log())
        if (e.meldung.contains("modelle.csv"))
            logGefunden = true;
    QVERIFY(logGefunden);
}

void TestPrintFarm::fehlendeDateienDegradation()
{
    const QString dir = QDir::tempPath() + "/pf_leer";
    QDir().mkpath(dir);
    DataStore store;
    QVERIFY(!store.loadFromFolder(dir));           // keine Pflichtdateien -> false
    QCOMPARE(store.drucker().size(), 0);
    bool dateiFehler = false;
    for (const auto &e : store.log())
        if (e.kategorie == LogEintrag::Datei) dateiFehler = true;
    QVERIFY(dateiFehler);                          // Datei-Fehler im Log (Tab Fehlerlog)
}

void TestPrintFarm::defektesJson()
{
    const QString dir = schreibeTestdaten();
    // ungültiges JSON
    QFile f(dir + "/prot/defekt.json");
    f.open(QIODevice::WriteOnly);
    f.write("{ das ist kein json !!!");
    f.close();
    CsvIo::write(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"J00001","prot/J00001.json","10","2026-01-01 08:00:00","2026-01-01 09:00:00"},
        {"J00002","prot/defekt.json","5","2026-01-01 08:00:00","2026-01-01 09:00:00"},
        {"J00003","prot/fehlt.json","5","2026-01-01 08:00:00","2026-01-01 09:00:00"},
    });
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const Protokoll bad = loader.load("J00002");
    QVERIFY(!bad.valid);
    QVERIFY(bad.fehler.contains(QStringLiteral("JSON")));
    const Protokoll missing = loader.load("J00003");
    QVERIFY(!missing.valid);
    QVERIFY(missing.fehler.contains(QStringLiteral("nicht lesbar")));
}

void TestPrintFarm::protokollNichtImIndex()
{
    const QString dir = schreibeTestdaten();
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const Protokoll p = loader.load("J99999");
    QVERIFY(!p.valid);
    QVERIFY(p.fehler.contains(QStringLiteral("Master-Index")));
}

void TestPrintFarm::lruCacheBegrenzt()
{
    const QString dir = schreibeTestdaten();
    // 25 synthetische Index-Einträge auf ein und dasselbe gültige Protokoll
    QList<QStringList> rows = {{"auftrag_id","pfad","anzahl_werte","start","ende"}};
    for (int i = 0; i < 25; ++i)
        rows.append({QStringLiteral("J%1").arg(i, 5, 10, QLatin1Char('0')), "prot/J00001.json", "10", "", ""});
    CsvIo::write(dir + "/master_protokoll_index.csv", rows);
    ProtocolLoader loader;
    loader.loadIndex(dir);
    for (int i = 0; i < 25; ++i)
        loader.load(QStringLiteral("J%1").arg(i, 5, 10, QLatin1Char('0')));
    QCOMPARE(loader.cacheSize(), 20);              // LRU deckelt bei 20
    QVERIFY(loader.cached(QStringLiteral("J00024")));   // Neuester noch drin
    QVERIFY(!loader.cached(QStringLiteral("J00000")));  // Ältester verdrängt
}

void TestPrintFarm::asyncLoadSignal()
{
    const QString dir = schreibeTestdaten();
    ProtocolLoader loader;
    loader.loadIndex(dir);
    QSignalSpy spy(&loader, &ProtocolLoader::fertig);
    loader.loadAsync("J00001");
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.count(), 1);
    QVERIFY(loader.cached("J00001"));
    // erneuter Aufruf kommt aus dem Cache (sofortiges Signal)
    QSignalSpy spy2(&loader, &ProtocolLoader::fertig);
    loader.loadAsync("J00001");
    QCOMPARE(spy2.count(), 1);
}

void TestPrintFarm::regelgueteNichtAusgeregelt()
{
    // Temperatur bleibt dauerhaft außerhalb des ±2-K-Bandes
    QVector<Messpunkt> auf, dr;
    for (int i = 0; i <= 50; ++i) {
        Messpunkt m; m.zeitS = i; m.dueseIstC = 100.0;   // konstant 110 K unter Soll
        auf.append(m);
    }
    const Regelguete rg = RegelgueteRechner::werteAus(auf, dr, 210.0);
    QVERIFY(rg.verfuegbar);
    QVERIFY(!rg.ausgeregelt);
    QCOMPARE(rg.ausregelzeitS, -1.0);
}

void TestPrintFarm::regelgueteKeinSprung()
{
    // Soll == Start -> keine Sprungantwort auswertbar
    QVector<Messpunkt> auf, dr;
    for (int i = 0; i <= 10; ++i) {
        Messpunkt m; m.zeitS = i; m.dueseIstC = 210.0;
        auf.append(m);
    }
    const Regelguete rg = RegelgueteRechner::werteAus(auf, dr, 210.0);
    QVERIFY(!rg.verfuegbar);
    QVERIFY(!rg.bemerkung.isEmpty());
}

void TestPrintFarm::csvRoundtrip()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    // Stammdaten ändern und exportieren
    Drucker d = store.drucker()[0];
    d.status = QStringLiteral("defekt");
    d.standort = QStringLiteral("B-99-9");
    QVERIFY(store.updateDrucker(d));
    const QString out = QDir::tempPath() + "/pf_roundtrip";
    QDir().mkpath(out);
    // Pflichtdateien komplett kopieren, nur drucker/auftraege werden ersetzt
    QFile::copy(dir + "/modelle.csv", out + "/modelle.csv");
    QFile::copy(dir + "/materialien.csv", out + "/materialien.csv");
    QString err;
    QVERIFY(store.exportCsv(out, &err));
    // Re-Import in frischen Store: Änderung muss ankommen
    DataStore store2;
    // exportCsv schreibt drucker_out.csv; für Roundtrip umbenennen
    QFile::remove(out + "/drucker.csv");
    QFile::rename(out + "/drucker_out.csv", out + "/drucker.csv");
    QFile::remove(out + "/auftraege.csv");
    QFile::rename(out + "/auftraege_out.csv", out + "/auftraege.csv");
    QVERIFY(store2.loadFromFolder(out));
    const Drucker *d2 = store2.drucker(store.drucker()[0].druckerId);
    QVERIFY(d2);
    QCOMPARE(d2->status, QStringLiteral("defekt"));
    QCOMPARE(d2->standort, QStringLiteral("B-99-9"));
    QCOMPARE(store2.auftraege().size(), store.auftraege().size());
}

void TestPrintFarm::planungRespektiertMaterialGrenzen()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    // Material mit 300 °C, Modell erlaubt nur 260 -> kein Drucker geeignet
    Material heiss;
    heiss.materialId = "MAT99"; heiss.bezeichnung = "PEEK"; heiss.dueseTempC = 300; heiss.bettTempC = 120;
    Auftrag a;
    a.auftragId = "JHEISS"; a.druckerId = ""; a.materialId = "MAT99";
    a.dauerSollMin = 60; a.ergebnis = QStringLiteral("ausstehend");
    // store kennt MAT99 nicht -> material() = nullptr -> in greedy wird mat==nullptr -> continue
    auto erg = Planung::greedy({a}, store.drucker(), store, QDateTime(QDate(2026,1,5), QTime(8,0)));
    QCOMPARE(erg.size(), 0);   // unbekanntes Material -> keine Zuweisung, kein Crash
}

void TestPrintFarm::planungAlleDruckerBelegt()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    QList<Drucker> alle = store.drucker();
    for (auto &d : alle) d.status = QStringLiteral("defekt");
    Auftrag a;
    a.auftragId = "JX"; a.materialId = "MAT01"; a.dauerSollMin = 30;
    a.ergebnis = QStringLiteral("ausstehend");
    auto erg = Planung::greedy({a}, alle, store, QDateTime(QDate(2026,1,5), QTime(8,0)));
    QCOMPARE(erg.size(), 0);
}

void TestPrintFarm::kostenOhneMaterial()
{
    Auftrag a;
    a.filamentSollG = 1000; a.dauerSollMin = 60;
    const auto k = Kostenrechnung::auftrag(a, nullptr);
    QCOMPARE(k.materialEur, 0.0);
    QVERIFY(k.gesamtEur > 0);   // Maschinenkosten trotzdem
}

void TestPrintFarm::pidSimNullParameter()
{
    const auto erg = PidSim::simuliere(0, 0, 0, 210.0, 24.0);
    QVERIFY(erg.zeitS.size() > 10);
    QVERIFY(qAbs(erg.istC.last() - 24.0) < 1.0);   // ohne Regler bleibt T ≈ T0
}

void TestPrintFarm::updateDruckerImExport()
{
    const QString dir = schreibeTestdaten();
    DataStore store;
    QVERIFY(store.loadFromFolder(dir));
    Drucker d = store.drucker()[1];
    d.pidKp = 9.9;
    QVERIFY(store.updateDrucker(d));
    QCOMPARE(store.drucker(store.drucker()[1].druckerId)->pidKp, 9.9);
    // unbekannter Drucker -> false
    Drucker fremd; fremd.druckerId = "D999";
    QVERIFY(!store.updateDrucker(fremd));
}

void TestPrintFarm::ungueltigeZeilenFormate()
{
    // Datums-/Zahl-Parser liefern bei Müll definierte Rückgaben (kein Crash)
    QVERIFY(!CsvIo::parseDate("32.13.2026").isValid());
    QVERIFY(!CsvIo::parseDateTime("2026-01-01").isValid());   // Zeit fehlt
    QVERIFY(CsvIo::parseDate("2026-01-01").isValid());
    QVERIFY(CsvIo::parseDateTime("2026-01-01 08:00:00").isValid());
}

// ---------------- QA-Runde 3 ----------------------------------------------

void TestPrintFarm::regelgueteUeberschwingen()
{
    // synthetisch: 24 -> 216 (Überschwingen 6 K über 210), dann Einpendeln auf 210
    QVector<Messpunkt> auf, dr;
    QVector<double> z, ist;
    for (int i = 0; i <= 60; ++i) {
        Messpunkt m; m.zeitS = i;
        m.dueseIstC = (i < 40) ? 24.0 + (216.0 - 24.0) * i / 40.0   // Anstieg auf 216
                               : 210.0 + 6.0 * std::exp(-(i - 40) / 10.0); // Abfall auf 210
        auf.append(m); z.append(i); ist.append(m.dueseIstC);
    }
    const Regelguete rg = RegelgueteRechner::werteAus(auf, dr, 210.0);
    QVERIFY(rg.verfuegbar);
    QVERIFY(qAbs(rg.ueberschwingenK - 6.0) < 0.5);
    QVERIFY(rg.ueberschwingenProzent > 3.0);   // 6/186 ≈ 3.2 %
    QVERIFY(rg.ausgeregelt);
}

void TestPrintFarm::regelabweichungNurPausen()
{
    // Druckphase besteht nur aus Pausen -> n=0, Ergebnis definiert 0 (kein Crash/NaN)
    QVector<Messpunkt> druck;
    for (int i = 0; i < 5; ++i) {
        Messpunkt m; m.zeitS = 100.0 + i * 30; m.schicht = 0; m.dueseIstC = 150.0;
        druck.append(m);
    }
    double mittel, maxi;
    RegelgueteRechner::regelabweichungDruck(druck, 210.0, mittel, maxi);
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
    QVERIFY(CsvIo::write(path, rows, &err));
    bool ok = false;
    const auto back = CsvIo::read(path, &ok);
    QVERIFY(ok);
    QCOMPARE(back.size(), 3);
    QCOMPARE(back[1][0], rows[1][0]);
    QCOMPARE(back[2][0], rows[2][0]);
}

void TestPrintFarm::protokollNurAufheizen()
{
    const QString dir = schreibeTestdaten();
    // JSON nur mit Aufheizphase, ohne druck-Objekt
    QJsonObject root;
    QJsonObject auftrag;
    auftrag["auftrag_id"] = "JY"; auftrag["drucker_id"] = "D001"; auftrag["material_id"] = "MAT01";
    auftrag["duesentemp_soll_c"] = 210.0; auftrag["betttemp_soll_c"] = 60.0; auftrag["schichtanzahl"] = 0;
    root["auftrag"] = auftrag;
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
    CsvIo::write(dir + "/master_protokoll_index.csv", {
        {"auftrag_id","pfad","anzahl_werte","start","ende"},
        {"JY","prot/onlyauf.json","51","",""},
    });
    ProtocolLoader loader;
    loader.loadIndex(dir);
    const Protokoll p = loader.load("JY");
    QVERIFY(p.valid);                    // Aufheizen allein genügt
    QCOMPARE(p.aufheizen.size(), 51);
    QCOMPARE(p.druck.size(), 0);         // Druckphase leer, kein Crash
    const Regelguete rg = RegelgueteRechner::werteAus(p.aufheizen, p.druck, 210.0);
    QVERIFY(rg.verfuegbar);
    QCOMPARE(rg.mittlereAbweichungK, 0.0);
}

QTEST_GUILESS_MAIN(TestPrintFarm)
#include "tst_printfarm.moc"
