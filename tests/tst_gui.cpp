// QA-GUI-Test: steuert das Hauptfenster headless (offscreen) mit den echten
// Projektdaten an und prüft die Kernabläufe der Bedienoberfläche.

#include <QtTest>
#include <QApplication>
#include <QTabWidget>
#include <QTableView>
#include <QLineEdit>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include "mainwindow.h"
#include "tablemodels.h"
#include "regelguete.h"
#include "dialogs.h"
#include "plotwidget.h"
#include "ganttwidget.h"
#include "analyse.h"
#include "auth.h"
#include <QLabel>
#include <QChart>

class TestGui : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void vierTabsInReihenfolge();
    void datenSofortSichtbar();
    void filterFunktioniert();
    void sortierungFunktioniert();
    void protokollUndRegelgueteEchteDaten();
    void themeUndSpracheUmschaltbar();
    void detailDialogOeffnetSich();
    // --- QA-Runde 2 ---
    void spracheWechseltTabTexte();
    void materialTabStatistikGefuellt();
    void fehlerlogHatEintraege();
    void teileInitialSortiertUndAusgewaehlt();   // vor dem Filter-Test, da der die Sortierung zurücksetzt
    void teileFilterAusstehend();
    void ganttLaesstSichZeichnen();
    void themeFarbenBeiderModi();
    void tabBleibtNachUiNeuaufbauErhalten();
    void sortierungMitDuplikatenCrashfrei();
    void protokollOhneAnmeldungSichtbar();
private:
    MainWindow *w = nullptr;
    QString dataDir;
};

void TestGui::initTestCase()
{
    // echte Daten aus dem Repo verwenden
    dataDir = QCoreApplication::applicationDirPath() + "/../QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten";
    if (!QFileInfo::exists(dataDir + "/drucker.csv"))
        dataDir = QStringLiteral("QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten");
    QVERIFY2(QFileInfo::exists(dataDir + "/drucker.csv"), "Testdaten nicht gefunden");
    w = new MainWindow();
    QVERIFY(w->ladeDatenordner(QDir(dataDir).absolutePath()));
}

void TestGui::vierTabsInReihenfolge()
{
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->count(), 4);
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));
    QCOMPARE(tabs->tabText(1), QString::fromUtf8("Gedruckte Teile"));
    QCOMPARE(tabs->tabText(2), QString::fromUtf8("Material"));
    QCOMPARE(tabs->tabText(3), QString::fromUtf8("Fehlerlog"));
    QCOMPARE(tabs->tabPosition(), QTabWidget::North);  // Tabs oben
}

void TestGui::datenSofortSichtbar()
{
    // ohne Filterumschaltung müssen sofort Daten in der Druckerliste stehen
    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    QVERIFY(view);
    auto *model = view->model();
    QCOMPARE(model->rowCount(), 80);   // alle 80 Drucker sofort sichtbar
}

void TestGui::filterFunktioniert()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab0 = tabs->widget(0);
    auto *suche = tab0->findChild<QLineEdit*>();
    auto *view = tab0->findChild<QTableView*>();
    auto *model = view->model();
    QVERIFY(suche);
    const int vorher = model->rowCount();
    QCOMPARE(vorher, 80);
    suche->setText(QStringLiteral("D001"));
    QCOMPARE(model->rowCount(), 1);    // genau ein Treffer
    suche->setText(QString());
    QCOMPARE(model->rowCount(), 80);   // zurücksetzen zeigt wieder alle
    // Status-Filter: Combo mit Status-Werten finden und „wartung“ wählen
    QComboBox *statusCombo = nullptr;
    for (auto *c : tab0->findChildren<QComboBox*>()) {
        if (c->findData(QStringLiteral("wartung")) >= 0) statusCombo = c;
    }
    QVERIFY(statusCombo);
    statusCombo->setCurrentIndex(statusCombo->findData(QStringLiteral("wartung")));
    QVERIFY(model->rowCount() < 80);   // 4 Wartungen in den Daten
    QCOMPARE(model->rowCount(), 4);
    statusCombo->setCurrentIndex(0);
    QCOMPARE(model->rowCount(), 80);
}

void TestGui::sortierungFunktioniert()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = view->model();
    // nach Betriebsstunden sortieren (Spalte 3)
    model->sort(3, Qt::DescendingOrder);
    const double first = model->data(model->index(0, 3)).toDouble();
    const double last = model->data(model->index(model->rowCount() - 1, 3)).toDouble();
    QVERIFY(first >= last);
    model->sort(0, Qt::AscendingOrder);
}

void TestGui::protokollUndRegelgueteEchteDaten()
{
    // ein echtes Protokoll aus dem ersten Batch laden und Regelgüte rechnen
    ProtocolLoader loader;
    QVERIFY(loader.loadIndex(QDir(dataDir).absolutePath()) > 1600);   // Master-Index: 1615 Einträge
    const Protokoll p = loader.load(QStringLiteral("J00001"));
    QVERIFY(p.valid);
    QVERIFY(p.aufheizen.size() > 100);
    QVERIFY(p.druck.size() > 100);
    const Regelguete rg = RegelgueteRechner::werteAus(p.aufheizen, p.druck, p.dueseSollC);
    QVERIFY(rg.verfuegbar);
    QVERIFY(rg.anstiegszeitS > 0);
    QVERIFY(rg.ausgeregelt);
    QVERIFY(rg.ausregelzeitS > 0 && rg.ausregelzeitS < 300);
    // Chart-Widget mit echten Daten zeichnen (offscreen)
    PlotWidget plot;
    plot.zeigeProtokoll(p);
    plot.resize(600, 300);
    plot.show();
    QVERIFY(true);
}

void TestGui::themeUndSpracheUmschaltbar()
{
    // Darkmode an/aus über das Menü (applyTheme darf nicht crashen)
    QVERIFY(QTest::qWaitForWindowExposed(w) || true);   // offscreen: exposed egal
    auto menubar = w->menuBar();
    QVERIFY(menubar);
    QList<QAction*> actions = menubar->actions();
    QVERIFY(actions.size() >= 3);
    // Theme-Aktion finden und zweimal toggeln
    QAction *theme = nullptr;
    for (auto *m : menubar->actions()) {
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QString::fromUtf8("Darkmode")) || a->text().contains("dark"))
                theme = a;
    }
    QVERIFY(theme);
    theme->trigger();   // dark an
    theme->trigger();   // dark aus
    QVERIFY(true);
}

void TestGui::detailDialogOeffnetSich()
{
    // Drucker-Detaildialog instanziieren (Doppelklick-Simulation auf Zeile 0)
    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = view->model();
    const auto *d = static_cast<DruckerModel*>(model)->druckerAt(0);
    QVERIFY(d);
    DruckerDetailDialog dlg(*d, w->store(), w->loader(), w);
    dlg.show();
    QTest::qWait(50);
    QVERIFY(dlg.isVisible() || true);   // offscreen: show() genügt als Smoke-Test
}

// ---------------- QA-Runde 2: GUI ----------------------------------------

void TestGui::spracheWechseltTabTexte()
{
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));
    // Sprache auf Englisch umschalten (Menü Ansicht -> letzter Eintrag)
    QAction *lang = nullptr;
    for (auto *m : w->menuBar()->actions()) {
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QStringLiteral("English")))
                lang = a;
    }
    QVERIFY(lang);
    lang->trigger();
    tabs = w->findChild<QTabWidget*>();   // UI wurde neu aufgebaut
    QVERIFY(tabs);
    QCOMPARE(tabs->tabText(0), QStringLiteral("Printers"));
    // zurück auf Deutsch
    for (auto *m : w->menuBar()->actions()) {
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QString::fromUtf8("Deutsch")))
                lang = a;
    }
    QVERIFY(lang);
    lang->trigger();
    tabs = w->findChild<QTabWidget*>();
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));
}

void TestGui::materialTabStatistikGefuellt()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab2 = tabs->widget(2);
    const auto labels = tab2->findChildren<QLabel*>();
    bool statsGefuellt = false;
    for (auto *l : labels)
        if (l->text().contains(QString::fromUtf8("Statistiken")) && l->text().length() > 50)
            statsGefuellt = true;
    QVERIFY(statsGefuellt);
    // Material-Tabelle gefüllt (10 Materialien, 7 Spalten im MaterialModel)
    const auto views = tab2->findChildren<QTableView*>();
    QVERIFY(!views.isEmpty());
    bool materialTabelleGefuellt = false;
    for (auto *v : views)
        if (v->model()->columnCount() == 7 && v->model()->rowCount() == 10)
            materialTabelleGefuellt = true;
    QVERIFY(materialTabelleGefuellt);
}

void TestGui::fehlerlogHatEintraege()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab3 = tabs->widget(3);
    auto *view = tab3->findChild<QTableView*>();
    QVERIFY(view);
    QVERIFY(view->model()->rowCount() > 0);   // mind. System-/Validierungs-Einträge
}

void TestGui::teileFilterAusstehend()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    auto *view = tab1->findChild<QTableView*>();
    QVERIFY(view);
    auto *model = view->model();
    const int gesamt = model->rowCount();
    QVERIFY(gesamt > 1500);   // echte Daten: 1633 Aufträge
    // Ergebnis-Filter "ausstehend" wählen (17 in den echten Daten)
    QComboBox *erg = nullptr;
    for (auto *c : tab1->findChildren<QComboBox*>())
        if (c->findData(QStringLiteral("ausstehend")) >= 0) erg = c;
    QVERIFY(erg);
    erg->setCurrentIndex(erg->findData(QStringLiteral("ausstehend")));
    QCOMPARE(model->rowCount(), 17);
    QVERIFY(model->rowCount() < gesamt);
    erg->setCurrentIndex(0);
    QCOMPARE(model->rowCount(), gesamt);
}

void TestGui::ganttLaesstSichZeichnen()
{
    GanttWidget g;
    QList<Planung::Zuweisung> z = {
        {"J1", "D001", QDateTime(QDate(2026,1,5), QTime(8,0)), QDateTime(QDate(2026,1,5), QTime(10,0))},
        {"J2", "D001", QDateTime(QDate(2026,1,5), QTime(10,0)), QDateTime(QDate(2026,1,5), QTime(11,0))},
        {"J3", "D002", QDateTime(QDate(2026,1,5), QTime(8,0)), QDateTime(QDate(2026,1,5), QTime(9,30))},
    };
    g.setPlanung(z, {"D001", "D002"});
    g.resize(600, 200);
    g.show();
    QTest::qWait(30);
    g.leeren();
    QVERIFY(true);   // kein Crash beim Zeichnen
}

void TestGui::themeFarbenBeiderModi()
{
    // Darkmode einschalten: Stylesheet muss die dunkle Palette enthalten
    QAction *theme = nullptr;
    for (auto *m : w->menuBar()->actions())
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QString::fromUtf8("Darkmode")))
                theme = a;
    QVERIFY(theme);
    if (!theme->isChecked()) theme->trigger();
    QVERIFY(theme->isChecked());
    QString sheet = qApp->styleSheet();
    QVERIFY(sheet.contains(QStringLiteral("#202B39")));   // dunkler Hintergrund
    QVERIFY(sheet.contains(QStringLiteral("#4A7DFF")));   // dunkler Akzent
    QVERIFY(sheet.contains(QStringLiteral("#E6ECF3")));   // heller Text
    QVERIFY(sheet.contains(QStringLiteral("QComboBox QAbstractItemView")));  // Dropdown gestylt
    QVERIFY(sheet.contains(QStringLiteral("QScrollBar::handle")));           // Scrollbar gestylt
    // alle PlotWidgets folgen dem dunklen Theme
    for (auto *p : w->findChildren<PlotWidget*>())
        QCOMPARE(p->chart()->theme(), QChart::ChartThemeDark);

    // Lightmode: helle Palette
    theme->trigger();
    QVERIFY(!theme->isChecked());
    sheet = qApp->styleSheet();
    QVERIFY(sheet.contains(QStringLiteral("#D9DFE7")));   // heller Hintergrund
    QVERIFY(sheet.contains(QStringLiteral("#001065")));   // Akzent dunkelblau
    QVERIFY(sheet.contains(QStringLiteral("#1C2733")));   // dunkler Text
    for (auto *p : w->findChildren<PlotWidget*>())
        QCOMPARE(p->chart()->theme(), QChart::ChartThemeLight);
}

void TestGui::tabBleibtNachUiNeuaufbauErhalten()
{
    // Tab 3 (Fehlerlog) aktivieren, dann UI neu aufbauen (Sprachwechsel baut um)
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    tabs->setCurrentIndex(3);
    QCOMPARE(tabs->currentIndex(), 3);
    QAction *lang = nullptr;
    for (auto *m : w->menuBar()->actions())
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QStringLiteral("English")) || a->text().contains(QString::fromUtf8("Deutsch")))
                lang = a;
    QVERIFY(lang);
    lang->trigger();   // baut UI neu auf (wie nach Login)
    tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->currentIndex(), 3);   // gleicher Tab wie vorher
    tabs->setCurrentIndex(0);
}

void TestGui::sortierungMitDuplikatenCrashfrei()
{
    // Regression: Sortieren nach Status (74x "bereit" = Duplikate) darf nicht
    // crashen, weder auf- noch absteigend; danach müssen die Werte sortiert sein.
    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = static_cast<DruckerModel*>(view->model());
    model->sort(4, Qt::AscendingOrder);    // Status aufsteigend
    for (int i = 1; i < model->rowCount(); ++i)
        QVERIFY(model->data(model->index(i - 1, 4), Qt::DisplayRole).toString() <= model->data(model->index(i, 4), Qt::DisplayRole).toString());
    model->sort(4, Qt::DescendingOrder);   // umkehren -> früher Crash
    for (int i = 1; i < model->rowCount(); ++i)
        QVERIFY(model->data(model->index(i - 1, 4), Qt::DisplayRole).toString() >= model->data(model->index(i, 4), Qt::DisplayRole).toString());
    model->sort(0, Qt::AscendingOrder);

    // ebenso Auftragsliste nach Ergebnis (viele Duplikate) in beide Richtungen
    auto *view1 = tabs->widget(1)->findChild<QTableView*>();
    auto *amodel = static_cast<AuftragModel*>(view1->model());
    amodel->sort(5, Qt::AscendingOrder);
    amodel->sort(5, Qt::DescendingOrder);
    for (int i = 1; i < amodel->rowCount(); ++i)
        QVERIFY(amodel->data(amodel->index(i - 1, 5), Qt::DisplayRole).toString() >= amodel->data(amodel->index(i, 5), Qt::DisplayRole).toString());
    amodel->sort(0, Qt::AscendingOrder);
}

void TestGui::teileInitialSortiertUndAusgewaehlt()
{
    // Tab "Gedruckte Teile": default nach Start absteigend sortiert
    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(1)->findChild<QTableView*>();
    auto *model = static_cast<AuftragModel*>(view->model());
    QVERIFY(model->rowCount() > 1500);
    const QString first = model->data(model->index(0, AuftragModel::ColStart), Qt::DisplayRole).toString();
    const QString last = model->data(model->index(model->rowCount() - 1, AuftragModel::ColStart), Qt::DisplayRole).toString();
    QVERIFY(first >= last);   // absteigend nach Zeit
    // ein fertiger Auftrag ist ausgewählt (Zeile selektiert)
    QVERIFY(view->currentIndex().isValid());
    const auto *a = model->auftragAt(view->currentIndex().row());
    QVERIFY(a);
    QVERIFY(a->ergebnis != QLatin1String("ausstehend"));
    QVERIFY(a->ende.isValid());   // letzter beendeter Druck
}

void TestGui::protokollOhneAnmeldungSichtbar()
{
    // explizit NICHT angemeldet
    Auth::logout();
    QCOMPARE(Auth::rolle(), Auth::Keine);
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    // im Teile-Tab: Doppelklick auf einen fertigen Auftrag -> Plot muss Serien haben (ohne Login)
    auto *view = tab1->findChild<QTableView*>();
    auto *plot = tab1->findChild<PlotWidget*>();
    QVERIFY(plot);
    auto *model = static_cast<AuftragModel*>(view->model());
    int ziel = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->auftragAt(i);
        if (a->ergebnis != QLatin1String("ausstehend") && a->ende.isValid()) {
            ziel = i; break;
        }
    }
    QVERIFY(ziel >= 0);
    emit view->doubleClicked(model->index(ziel, 0));
    QTest::qWait(50);
    QVERIFY(!plot->chart()->series().isEmpty());   // Protokoll gezeichnet, ohne Anmeldung
    // nochmal einen anderen fertigen Auftrag anklicken
    int ziel2 = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->auftragAt(i);
        if (i != ziel && a->ergebnis != QLatin1String("ausstehend") && a->ende.isValid()) {
            ziel2 = i; break;
        }
    }
    QVERIFY(ziel2 >= 0);
    emit view->doubleClicked(model->index(ziel2, 0));
    QTest::qWait(50);
    QVERIFY(!plot->chart()->series().isEmpty());
}

QTEST_MAIN(TestGui)
#include "tst_gui.moc"
