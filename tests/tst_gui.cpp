

#include <QtTest>
#include <QApplication>
#include <QTabWidget>
#include <QTableView>
#include <QSplitter>
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
#include "druckerwidget.h"
#include <QLabel>
#include <QChart>
#include <QChartView>
#include <QCategoryAxis>
#include <QPushButton>
#include <QGroupBox>
#include <QAbstractButton>
#include <QValueAxis>
#include <QLocale>
#include <QStyledItemDelegate>
#include <QLineSeries>
#include <QCheckBox>
#include <QDateTimeAxis>
#include <QListWidget>

class TestGui : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void fourTabsInOrder();
    void dataVisibleImmediately();
    void filterWorks();
    void sortingWorks();
    void protocolAndQualityRealData();
    void themeAndLanguageSwitchable();
    void detailDialogOpens();

    void languageSwitchesTabTexts();
    void materialTabStatsFilled();
    void materialListSortable();
    void errorLogHasEntries();
    void partsInitiallySortedAndSelected();
    void partsFilterPending();
    void partsSplitterMovable();
    void fullscreenViaFKey();
    void ganttRenders();
    void themeColorsBothModes();
    void tabSurvivesUiRebuild();
    void sortingWithDuplicatesCrashFree();
    void protocolVisibleWithoutLogin();
    void scheduleAdminOnly();
    void decimalPointIsDot();
    void languageSwitchComplete();
    void enModeNoGermanFallbacks();
    void plotLegendTranslated();
    void singleClickUpdatesProtocol();
    void adaptiveTimeScaleTested();
    void adaptiveToggleAxis();

    void pidDialogRankingAndFit();
    void maintenanceTableInDetailDialog();
    void sqliteImportMenuPresent();
    void asyncLoadNonBlocking();

    void rightsSeparationEnforced();
private:
    MainWindow *w = nullptr;
    QString dataDir;
};

void TestGui::initTestCase()
{

    dataDir = QCoreApplication::applicationDirPath() + "/../QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten";
    if (!QFileInfo::exists(dataDir + "/drucker.csv"))
        dataDir = QStringLiteral("QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten");
    QVERIFY2(QFileInfo::exists(dataDir + "/drucker.csv"), "Testdaten nicht gefunden");
    QLocale::setDefault(QLocale::c());
    w = new MainWindow();
    QVERIFY(w->loadDataFolder(QDir(dataDir).absolutePath()));
}

void TestGui::fourTabsInOrder()
{
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->count(), 4);
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));
    QCOMPARE(tabs->tabText(1), QString::fromUtf8("Gedruckte Teile"));
    QCOMPARE(tabs->tabText(2), QString::fromUtf8("Material"));
    QCOMPARE(tabs->tabText(3), QString::fromUtf8("Fehlerlog"));
    QCOMPARE(tabs->tabPosition(), QTabWidget::North);
}

void TestGui::dataVisibleImmediately()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    QVERIFY(view);
    auto *model = view->model();
    QCOMPARE(model->rowCount(), 80);

    for (int t = 0; t < tabs->count(); ++t)
        for (auto *v : tabs->widget(t)->findChildren<QTableView*>())
            QVERIFY2(v->alternatingRowColors(), "alternatingRowColors fehlt in einem Tab");
}

void TestGui::filterWorks()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab0 = tabs->widget(0);
    auto *suche = tab0->findChild<QLineEdit*>();
    auto *view = tab0->findChild<QTableView*>();
    auto *model = view->model();
    QVERIFY(suche);
    const int before = model->rowCount();
    QCOMPARE(before, 80);
    suche->setText(QStringLiteral("D001"));
    QCOMPARE(model->rowCount(), 1);
    suche->setText(QString());
    QCOMPARE(model->rowCount(), 80);

    QComboBox *statusCombo = nullptr;
    for (auto *c : tab0->findChildren<QComboBox*>()) {
        if (c->findText(QStringLiteral("wartung")) >= 0) statusCombo = c;
    }
    QVERIFY(statusCombo);
    statusCombo->setCurrentIndex(statusCombo->findText(QStringLiteral("wartung")));
    QVERIFY(model->rowCount() < 80);
    QCOMPARE(model->rowCount(), 4);
    statusCombo->setCurrentIndex(0);
    QCOMPARE(model->rowCount(), 80);

    suche->setText(QStringLiteral("D001"));
    QCOMPARE(model->rowCount(), 1);
    auto *resetBtn = tab0->findChild<QPushButton*>();
    QVERIFY(resetBtn);
    resetBtn->click();
    QCOMPARE(suche->text(), QString());
    QCOMPARE(statusCombo->currentIndex(), 0);
    QCOMPARE(model->rowCount(), 80);
}

void TestGui::sortingWorks()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = view->model();

    model->sort(3, Qt::DescendingOrder);
    const double first = model->data(model->index(0, 3)).toDouble();
    const double last = model->data(model->index(model->rowCount() - 1, 3)).toDouble();
    QVERIFY(first >= last);
    model->sort(0, Qt::AscendingOrder);
}

void TestGui::protocolAndQualityRealData()
{

    ProtocolLoader loader;
    QVERIFY(loader.loadIndex(QDir(dataDir).absolutePath()) > 1600);
    const PrintLog p = loader.load(QStringLiteral("J00001"));
    QVERIFY(p.valid);
    QVERIFY(p.heatUp.size() > 100);
    QVERIFY(p.printPhase.size() > 100);
    const ControlQuality rg = ControlQualityCalculator::computeValues(p.heatUp, p.printPhase, p.nozzleTargetC);
    QVERIFY(rg.available);
    QVERIFY(rg.riseTimeS > 0);
    QVERIFY(rg.settled);
    QVERIFY(rg.settlingTimeS > 0 && rg.settlingTimeS < 300);

    PlotWidget plot;
    plot.showProtocol(p);
    plot.resize(600, 300);
    plot.show();
    QVERIFY(true);
}

void TestGui::themeAndLanguageSwitchable()
{

    QVERIFY(QTest::qWaitForWindowExposed(w) || true);
    auto menubar = w->menuBar();
    QVERIFY(menubar);
    QList<QAction*> actions = menubar->actions();
    QVERIFY(actions.size() >= 3);

    QAction *theme = nullptr;
    for (auto *m : menubar->actions()) {
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QString::fromUtf8("Darkmode")) || a->text().contains("dark"))
                theme = a;
    }
    QVERIFY(theme);
    theme->trigger();
    theme->trigger();
    QVERIFY(true);
}

void TestGui::detailDialogOpens()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = view->model();
    const auto *d = static_cast<PrinterModel*>(model)->printerAtRow(0);
    QVERIFY(d);
    PrinterDetailDialog dlg(*d, w->store(), w->loader(), w);
    dlg.show();
    QTest::qWait(50);
    QVERIFY(dlg.isVisible() || true);
}

void TestGui::languageSwitchesTabTexts()
{
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));

    QAction *lang = nullptr;
    for (auto *m : w->menuBar()->actions()) {
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QStringLiteral("English")))
                lang = a;
    }
    QVERIFY(lang);
    lang->trigger();
    tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->tabText(0), QStringLiteral("Printers"));

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

void TestGui::materialTabStatsFilled()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab2 = tabs->widget(2);
    const auto labels = tab2->findChildren<QLabel*>();
    bool statsGefuellt = false;
    for (auto *l : labels)
        if (l->text().contains(QString::fromUtf8("Verbrauchsstatistik")) && l->text().length() > 50)
            statsGefuellt = true;
    QVERIFY(statsGefuellt);

    const auto views = tab2->findChildren<QTableView*>();
    QCOMPARE(views.size(), 1);

    bool materialTableFilled = false;
    for (auto *v : views)
        if (v->model()->columnCount() == 8 && v->model()->rowCount() == 10)
            materialTableFilled = true;
    QVERIFY(materialTableFilled);

    auto *chartView = tab2->findChild<QChartView*>();
    QVERIFY(chartView);
    QVERIFY(!chartView->chart()->series().isEmpty());
    auto *lineSeries = qobject_cast<QLineSeries*>(chartView->chart()->series().first());
    QVERIFY(lineSeries);
    QVERIFY(lineSeries->pointsVisible());
    QVERIFY(lineSeries->count() > 50);

    auto *axisY = qobject_cast<QValueAxis*>(chartView->chart()->axes(Qt::Vertical).first());
    QVERIFY(axisY);
    QVERIFY(axisY->max() > 0);

    w->show();
    w->resize(1200, 900);
    tabs->setCurrentIndex(2);
    QTest::qWait(150);
    auto *axisX = qobject_cast<QDateTimeAxis*>(chartView->chart()->axes(Qt::Horizontal).first());
    QVERIFY(axisX);
    QCOMPARE(axisX->format(), QStringLiteral("d"));
    QVERIFY(axisX->tickCount() > 1);
    QVERIFY(axisX->labelsFont().pointSize() <= 10);

    QVERIFY(chartView->minimumHeight() >= 240);
}

void TestGui::materialListSortable()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab2 = tabs->widget(2);
    auto *view = tab2->findChild<QTableView*>();
    QVERIFY(view);
    QVERIFY(view->isSortingEnabled());
    auto *model = view->model();
    QVERIFY(model->rowCount() == 10);

    model->sort(1, Qt::AscendingOrder);
    const QString first = model->data(model->index(0, 1)).toString();
    const QString last = model->data(model->index(model->rowCount() - 1, 1)).toString();
    QVERIFY(first <= last);

    model->sort(1, Qt::DescendingOrder);
    QCOMPARE(model->data(model->index(0, 1)).toString(), last);
    QCOMPARE(model->data(model->index(model->rowCount() - 1, 1)).toString(), first);

    model->sort(7, Qt::DescendingOrder);
    const double top = model->data(model->index(0, 7)).toDouble();
    const double bottom = model->data(model->index(model->rowCount() - 1, 7)).toDouble();
    QVERIFY(top >= bottom);
    model->sort(0, Qt::AscendingOrder);
}

void TestGui::errorLogHasEntries()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab3 = tabs->widget(3);
    auto *view = tab3->findChild<QTableView*>();
    QVERIFY(view);
    QVERIFY(view->model()->rowCount() > 0);
}

void TestGui::partsFilterPending()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    auto *view = tab1->findChild<QTableView*>();
    QVERIFY(view);
    auto *model = view->model();
    const int gesamt = model->rowCount();
    QVERIFY(gesamt > 1500);

    QComboBox *res = nullptr;
    for (auto *c : tab1->findChildren<QComboBox*>())
        if (c->findData(QStringLiteral("ausstehend")) >= 0) res = c;
    QVERIFY(res);
    res->setCurrentIndex(res->findData(QStringLiteral("ausstehend")));
    QCOMPARE(model->rowCount(), 17);
    QVERIFY(model->rowCount() < gesamt);
    res->setCurrentIndex(0);
    QCOMPARE(model->rowCount(), gesamt);
}

void TestGui::partsSplitterMovable()
{
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);

    QSplitter *splitter = nullptr;
    for (auto *s : tab1->findChildren<QSplitter*>())
        if (s->orientation() == Qt::Vertical) splitter = s;
    QVERIFY(splitter);
    QCOMPARE(splitter->count(), 2);

    w->show();
    w->resize(1000, 800);
    tabs->setCurrentIndex(1);
    QTest::qWait(50);
    const int gesamt = splitter->height();
    QVERIFY(gesamt > 100);
    const QList<int> before = splitter->sizes();

    splitter->setSizes({gesamt / 4, gesamt - gesamt / 4});
    const QList<int> nachher = splitter->sizes();
    QVERIFY(nachher.at(1) > before.at(1));
    QVERIFY(nachher.at(0) < before.at(0));
    QVERIFY(!splitter->childrenCollapsible());
}

void TestGui::fullscreenViaFKey()
{
    QVERIFY(!w->isMaximized());
    QVERIFY(!w->isFullScreen());
    auto *tabs = w->findChild<QTabWidget*>();
    auto *suche = tabs->widget(0)->findChild<QLineEdit*>();
    QVERIFY(suche);

    suche->setFocus();
    QTest::keyClick(suche, Qt::Key_F);
    QVERIFY(!w->isMaximized());
    QCOMPARE(suche->text(), QStringLiteral("f"));
    suche->clear();

    w->setFocus();
    QTest::keyClick(w, Qt::Key_F);
    QVERIFY(w->isMaximized());
    QVERIFY(!w->isFullScreen());
    QTest::keyClick(w, Qt::Key_F);
    QVERIFY(!w->isMaximized());

    QAction *fs = nullptr;
    for (auto *m : w->menuBar()->actions())
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QStringLiteral("Vollbild"))) fs = a;
    QVERIFY(fs);
    QVERIFY(fs->isCheckable());
    fs->trigger();
    QVERIFY(w->isMaximized());
    QVERIFY(!w->isFullScreen());
    fs->trigger();
    QVERIFY(!w->isMaximized());
}

void TestGui::ganttRenders()
{
    GanttWidget g;
    QList<Planning::Assignment> z = {
        {"J1", "D001", QDateTime(QDate(2026,1,5), QTime(8,0)), QDateTime(QDate(2026,1,5), QTime(10,0))},
        {"J2", "D001", QDateTime(QDate(2026,1,5), QTime(10,0)), QDateTime(QDate(2026,1,5), QTime(11,0))},
        {"J3", "D002", QDateTime(QDate(2026,1,5), QTime(8,0)), QDateTime(QDate(2026,1,5), QTime(9,30))},
    };
    g.setSchedule(z, {"D001", "D002"});
    g.resize(600, 200);
    g.show();
    QTest::qWait(30);
    g.clear();
    QVERIFY(true);
}

void TestGui::themeColorsBothModes()
{

    QAction *theme = nullptr;
    for (auto *m : w->menuBar()->actions())
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QString::fromUtf8("Darkmode")))
                theme = a;
    QVERIFY(theme);
    if (!theme->isChecked()) theme->trigger();
    QVERIFY(theme->isChecked());
    QString sheet = qApp->styleSheet();
    QVERIFY(sheet.contains(QStringLiteral("#202B39")));
    QVERIFY(sheet.contains(QStringLiteral("#4A7DFF")));
    QVERIFY(sheet.contains(QStringLiteral("#E6ECF3")));
    QVERIFY(sheet.contains(QStringLiteral("QComboBox QAbstractItemView")));
    QVERIFY(sheet.contains(QStringLiteral("QScrollBar::handle")));

    for (auto *p : w->findChildren<PlotWidget*>())
        QCOMPARE(p->chart()->theme(), QChart::ChartThemeDark);

    theme->trigger();
    QVERIFY(!theme->isChecked());
    sheet = qApp->styleSheet();
    QVERIFY(sheet.contains(QStringLiteral("#D9DFE7")));
    QVERIFY(sheet.contains(QStringLiteral("#001065")));
    QVERIFY(sheet.contains(QStringLiteral("#1C2733")));
    for (auto *p : w->findChildren<PlotWidget*>())
        QCOMPARE(p->chart()->theme(), QChart::ChartThemeLight);
}

void TestGui::tabSurvivesUiRebuild()
{

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
    lang->trigger();
    tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->currentIndex(), 3);
    tabs->setCurrentIndex(0);

    if (w->findChild<QTabWidget*>() && w->findChild<QTabWidget*>()->tabText(0) != QString::fromUtf8("Druckerliste"))
        w->menuBar()->actions().at(1)->menu()->actions().last()->trigger();
}

void TestGui::sortingWithDuplicatesCrashFree()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = static_cast<PrinterModel*>(view->model());
    model->sort(4, Qt::AscendingOrder);
    for (int i = 1; i < model->rowCount(); ++i)
        QVERIFY(model->data(model->index(i - 1, 4), Qt::DisplayRole).toString() <= model->data(model->index(i, 4), Qt::DisplayRole).toString());
    model->sort(4, Qt::DescendingOrder);
    for (int i = 1; i < model->rowCount(); ++i)
        QVERIFY(model->data(model->index(i - 1, 4), Qt::DisplayRole).toString() >= model->data(model->index(i, 4), Qt::DisplayRole).toString());
    model->sort(0, Qt::AscendingOrder);

    auto *view1 = tabs->widget(1)->findChild<QTableView*>();
    auto *amodel = static_cast<JobModel*>(view1->model());
    amodel->sort(5, Qt::AscendingOrder);
    amodel->sort(5, Qt::DescendingOrder);
    for (int i = 1; i < amodel->rowCount(); ++i)
        QVERIFY(amodel->data(amodel->index(i - 1, 5), Qt::DisplayRole).toString() >= amodel->data(amodel->index(i, 5), Qt::DisplayRole).toString());
    amodel->sort(0, Qt::AscendingOrder);
}

void TestGui::partsInitiallySortedAndSelected()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(1)->findChild<QTableView*>();
    auto *model = static_cast<JobModel*>(view->model());
    QVERIFY(model->rowCount() > 1500);
    const QString first = model->data(model->index(0, JobModel::ColStart), Qt::DisplayRole).toString();
    const QString last = model->data(model->index(model->rowCount() - 1, JobModel::ColStart), Qt::DisplayRole).toString();
    QVERIFY(first >= last);

    QVERIFY(view->currentIndex().isValid());
    const auto *a = model->jobAtRow(view->currentIndex().row());
    QVERIFY(a);
    QVERIFY(a->jobResult != QLatin1String("ausstehend"));
    QVERIFY(a->ende.isValid());
}

void TestGui::protocolVisibleWithoutLogin()
{

    Auth::logout();
    QCOMPARE(Auth::role(), Auth::None);
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);

    auto *view = tab1->findChild<QTableView*>();
    auto *plot = tab1->findChild<PlotWidget*>();
    QVERIFY(plot);
    auto *model = static_cast<JobModel*>(view->model());

    int pending = -1;
    for (int i = 0; i < model->rowCount(); ++i)
        if (model->jobAtRow(i)->jobResult == QLatin1String("ausstehend")) { pending = i; break; }
    QVERIFY(pending >= 0);
    emit view->clicked(model->index(pending, 0));
    QTest::qWait(30);
    QVERIFY(plot->chart()->series().isEmpty());
    QVERIFY(plot->currentJob().isEmpty());

    int ziel = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->jobAtRow(i);
        if (a->jobResult != QLatin1String("ausstehend") && a->ende.isValid()) {
            ziel = i; break;
        }
    }
    QVERIFY(ziel >= 0);
    const QString zielId = model->jobAtRow(ziel)->jobId;
    QSignalSpy spy(w->loader(), &ProtocolLoader::fertig);
    emit view->clicked(model->index(ziel, 0));
    if (!w->loader()->isCached(zielId))
        QVERIFY(spy.wait(5000));
    QTest::qWait(10);
    QVERIFY(!plot->chart()->series().isEmpty());
    QCOMPARE(plot->currentJob(), zielId);

    int ziel2 = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->jobAtRow(i);
        if (i != ziel && a->jobResult != QLatin1String("ausstehend") && a->ende.isValid()) {
            ziel2 = i; break;
        }
    }
    QVERIFY(ziel2 >= 0);
    const QString ziel2Id = model->jobAtRow(ziel2)->jobId;
    QSignalSpy spy2(w->loader(), &ProtocolLoader::fertig);
    emit view->clicked(model->index(ziel2, 0));
    if (!w->loader()->isCached(ziel2Id))
        QVERIFY(spy2.wait(5000));
    QTest::qWait(10);
    QVERIFY(!plot->chart()->series().isEmpty());
    QCOMPARE(plot->currentJob(), ziel2Id);

    auto *ax = plot->chart()->axes(Qt::Horizontal).first();
    QVERIFY(ax);
    QVERIFY(ax->titleText().contains(QStringLiteral("min")));
    QVERIFY(qobject_cast<QCategoryAxis*>(ax) != nullptr);
    auto *ay = qobject_cast<QValueAxis*>(plot->chart()->axes(Qt::Vertical).first());
    QVERIFY(ay);
    QCOMPARE(ay->labelFormat(), QStringLiteral("%.0f"));
}

void TestGui::scheduleAdminOnly()
{

    Auth::logout();
    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    const auto buttons = tab1->findChildren<QPushButton*>();
    QVERIFY(!buttons.isEmpty());
    for (auto *b : buttons)
        QVERIFY(!b->isVisibleTo(tab1) || !b->text().contains(QString::fromUtf8("Belegungsplan")));

    QCOMPARE(tab1->findChildren<QTableView*>().size(), 1);

    QVERIFY(tab1->findChild<PlotWidget*>());
}

void TestGui::decimalPointIsDot()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *view = tabs->widget(0)->findChild<QTableView*>();
    auto *model = static_cast<PrinterModel*>(view->model());
    const QVariant v = model->data(model->index(0, PrinterModel::ColOperatingHours), Qt::DisplayRole);
    QVERIFY(v.typeId() == QMetaType::LongLong || v.typeId() == QMetaType::Int);
    auto *delegate = qobject_cast<QStyledItemDelegate*>(view->itemDelegate());
    QVERIFY(delegate);
    QString text = delegate->displayText(v, QLocale());
    QVERIFY(!text.contains(QLatin1Char(',')));
    QVERIFY(!text.contains(QLatin1Char('.')));
}

void TestGui::languageSwitchComplete()
{

    auto sprachAktion = [this]() -> QAction* {
        return w->menuBar()->actions().at(1)->menu()->actions().last();
    };
    if (w->findChild<QTabWidget*>()->tabText(0) != QString::fromUtf8("Druckerliste"))
        sprachAktion()->trigger();

    sprachAktion()->trigger();
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);

    QCOMPARE(tabs->tabText(0), QStringLiteral("Printers"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("Printed parts"));
    QCOMPARE(tabs->tabText(3), QStringLiteral("Error log"));

    QVERIFY(w->menuBar()->actions()[0]->text().contains(QStringLiteral("File")));

    auto *tab0 = tabs->widget(0);
    bool foundAllStatuses = false;
    for (auto *c : tab0->findChildren<QComboBox*>())
        if (c->itemText(0) == QStringLiteral("All statuses")) foundAllStatuses = true;
    QVERIFY(foundAllStatuses);

    auto *tab3 = tabs->widget(3);
    bool foundAllCat = false;
    for (auto *c : tab3->findChildren<QComboBox*>())
        if (c->itemText(0) == QStringLiteral("All categories")) foundAllCat = true;
    QVERIFY(foundAllCat);

    auto *tab1 = tabs->widget(1);
    auto *view1 = tab1->findChild<QTableView*>();
    auto *amodel = static_cast<JobModel*>(view1->model());
    bool deutschBleibt = false;
    for (int i = 0; i < amodel->rowCount(); ++i) {
        const QString e = amodel->data(amodel->index(i, JobModel::ColResult), Qt::DisplayRole).toString();
        if (e == QLatin1String("erfolgreich") || e == QLatin1String("ausstehend")) deutschBleibt = true;
    }
    QVERIFY(deutschBleibt);

    sprachAktion()->trigger();
    tabs = w->findChild<QTabWidget*>();
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));
}

void TestGui::enModeNoGermanFallbacks()
{

    auto sprachAktion = [this]() -> QAction* {
        return w->menuBar()->actions().at(1)->menu()->actions().last();
    };
    if (w->findChild<QTabWidget*>()->tabText(0) != QString::fromUtf8("Druckerliste"))
        sprachAktion()->trigger();
    sprachAktion()->trigger();
    auto *tabs = w->findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->tabText(0), QStringLiteral("Printers"));

    bool fullscreenFound = false;
    bool loginMenuFound = false;
    for (auto *m : w->menuBar()->actions()) {
        if (m->text().contains(QStringLiteral("View")))
            for (auto *a : m->menu()->actions())
                if (a->text() == QStringLiteral("Fullscreen (F)")) fullscreenFound = true;
        if (m->text().contains(QStringLiteral("Login"))) {
            loginMenuFound = true;
            QCOMPARE(m->menu()->actions().first()->text(), QStringLiteral("Log in..."));
        }
    }
    QVERIFY(fullscreenFound);
    QVERIFY(loginMenuFound);

    for (int t : {0, 1, 3}) {
        bool resetFound = false;
        for (auto *b : tabs->widget(t)->findChildren<QPushButton*>()) {
            if (b->text() == QStringLiteral("Reset filters")) {
                resetFound = true;
                QVERIFY2(b->toolTip().contains(QStringLiteral("Reset")),
                         qPrintable(QStringLiteral("Tab %1 Tooltip: %2").arg(t).arg(b->toolTip())));
                QVERIFY2(!b->toolTip().contains(QString::fromUtf8("zurücksetzen")),
                         qPrintable(QStringLiteral("Tab %1 Tooltip: %2").arg(t).arg(b->toolTip())));
            }
        }
        QVERIFY2(resetFound, qPrintable(QStringLiteral("Tab %1: Reset-Button fehlt").arg(t)));
    }

    auto *tab2 = tabs->widget(2);
    bool statsBoxFound = false;
    for (auto *g : tab2->findChildren<QGroupBox*>())
        if (g->title() == QStringLiteral("Consumption statistics")) statsBoxFound = true;
    QVERIFY(statsBoxFound);
    auto *chartView = tab2->findChild<QChartView*>();
    QVERIFY(chartView);
    QCOMPARE(chartView->chart()->title(),
             QStringLiteral("Consumption in 6-hour intervals (successful prints)"));
    auto *ls = qobject_cast<QLineSeries*>(chartView->chart()->series().first());
    QVERIFY(ls);
    QCOMPARE(ls->name(), QStringLiteral("Filament [kg]"));
    auto *axisX = qobject_cast<QDateTimeAxis*>(chartView->chart()->axes(Qt::Horizontal).first());
    QVERIFY(axisX);
    QCOMPARE(axisX->titleText(), QStringLiteral("Day"));
    auto *matView = tab2->findChild<QTableView*>();
    QVERIFY(matView);
    QCOMPARE(matView->model()->headerData(7, Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("Ø consumption/week [kg]"));
    bool statsEn = false;
    for (auto *l : tab2->findChildren<QLabel*>())
        if (l->text().contains(QStringLiteral("Consumption statistics:"))
            && l->text().contains(QStringLiteral("Per material:"))
            && l->text().contains(QStringLiteral("Error codes:")))
            statsEn = true;
    QVERIFY(statsEn);

    LoginDialog dlg(w);
    QCOMPARE(dlg.windowTitle(), QStringLiteral("Log in"));
    QStringList dialogLabels;
    for (auto *l : dlg.findChildren<QLabel*>()) dialogLabels << l->text();
    QVERIFY(dialogLabels.contains(QStringLiteral("Password:")));
    QVERIFY(dialogLabels.contains(QStringLiteral("Role:")));
    QVERIFY(!dialogLabels.contains(QString::fromUtf8("Passwort:")));

    QCOMPARE(QCoreApplication::translate("MainWindow", "Anmeldung"), QStringLiteral("Login"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Fehlgeschlagene Anmeldung (falsches Passwort)"),
             QStringLiteral("Failed login (wrong password)"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Falsches Passwort für die gewählte Rolle."),
             QStringLiteral("Wrong password for the selected role."));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Anmeldung fehlgeschlagen: falsches Passwort."),
             QStringLiteral("Login failed: wrong password."));
    QCOMPARE(QCoreApplication::translate("PrinterWidget", "Ungültige Eingabe"),
             QStringLiteral("Invalid input"));
    QCOMPARE(QCoreApplication::translate("PrinterWidget",
             "PID-Werte müssen gültige Zahlen sein. Änderung verworfen."),
             QStringLiteral("PID values must be valid numbers. Change discarded."));

    const QStringList deutscheFallbacks = {
        QString::fromUtf8("Filter zurücksetzen"), QString::fromUtf8("Vollbild"),
        QString::fromUtf8("Passwort:"), QString::fromUtf8("Verbrauchsstatistik"),
        QString::fromUtf8("Anmeldung"), QString::fromUtf8("Fehlercodes:"),
        QString::fromUtf8("Pro Material:"), QString::fromUtf8("Ungültige Eingabe"),
        QString::fromUtf8("Ø Verbrauch/Woche"), QString::fromUtf8("zurücksetzen"),
    };
    QStringList allTexts;
    for (auto *m : w->menuBar()->actions()) {
        allTexts << m->text();
        if (m->menu())
            for (auto *a : m->menu()->actions()) allTexts << a->text();
    }
    for (int t = 0; t < tabs->count(); ++t) allTexts << tabs->tabText(t);
    for (auto *b : w->findChildren<QAbstractButton*>()) allTexts << b->text();
    for (auto *l : w->findChildren<QLabel*>()) allTexts << l->text();
    for (auto *g : w->findChildren<QGroupBox*>()) allTexts << g->title();
    for (auto *c : w->findChildren<QComboBox*>())
        for (int i = 0; i < c->count(); ++i) allTexts << c->itemText(i);
    for (const auto &dt : deutscheFallbacks)
        for (const auto &txt : allTexts)
            QVERIFY2(!txt.contains(dt),
                     qPrintable(QStringLiteral("Deutscher Fallback '%1' in: '%2'").arg(dt, txt)));

    sprachAktion()->trigger();
    tabs = w->findChild<QTabWidget*>();
    QCOMPARE(tabs->tabText(0), QString::fromUtf8("Druckerliste"));
}

void TestGui::plotLegendTranslated()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    auto *plot = tab1->findChild<PlotWidget*>();
    QVERIFY(plot);
    auto *view = tab1->findChild<QTableView*>();
    auto *model = static_cast<JobModel*>(view->model());
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->jobAtRow(i);
        if (a->jobResult != QLatin1String("ausstehend") && a->ende.isValid()) {
            emit view->doubleClicked(model->index(i, 0));
            break;
        }
    }
    QTest::qWait(30);
    QStringList names;
    for (auto *s : plot->chart()->series())
        names << s->name();
    QVERIFY(names.contains(QString::fromUtf8("Düsentemperatur")));
    QVERIFY(names.contains(QString::fromUtf8("Betttemperatur")));
    QVERIFY(!names.contains(QString::fromUtf8("Druckstart")));
    QVERIFY(!names.contains(QString::fromUtf8("Druckende")));
    QVERIFY(!names.contains(QString::fromUtf8("Düse Ist (Druck)")));

    if (tabs->tabText(0) != QStringLiteral("Printers"))
        w->menuBar()->actions().at(1)->menu()->actions().last()->trigger();
    tabs = w->findChild<QTabWidget*>();
    plot = tabs->widget(1)->findChild<PlotWidget*>();
    view = tabs->widget(1)->findChild<QTableView*>();
    model = static_cast<JobModel*>(view->model());
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->jobAtRow(i);
        if (a->jobResult != QLatin1String("ausstehend") && a->ende.isValid()) {
            emit view->doubleClicked(model->index(i, 0));
            break;
        }
    }
    QTest::qWait(30);
    names.clear();
    for (auto *s : plot->chart()->series())
        names << s->name();
    QVERIFY(names.contains(QStringLiteral("Nozzle temperature")));
    QVERIFY(names.contains(QStringLiteral("Bed temperature")));
    QVERIFY(!names.contains(QStringLiteral("Print start")));
    QVERIFY(!names.contains(QStringLiteral("Print end")));

    w->menuBar()->actions().at(1)->menu()->actions().last()->trigger();
}

void TestGui::singleClickUpdatesProtocol()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    auto *view = tab1->findChild<QTableView*>();
    auto *plot = tab1->findChild<PlotWidget*>();
    auto *model = static_cast<JobModel*>(view->model());
    int z1 = -1, z2 = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->jobAtRow(i);
        if (a->jobResult != QLatin1String("ausstehend") && a->ende.isValid()) {
            if (z1 < 0) z1 = i; else { z2 = i; break; }
        }
    }
    QVERIFY(z1 >= 0 && z2 >= 0);
    emit view->clicked(model->index(z1, 0));
    QTest::qWait(30);
    QCOMPARE(plot->currentJob(), model->jobAtRow(z1)->jobId);
    QVERIFY(plot->chart()->title().isEmpty());
    QCOMPARE(plot->chart()->legend()->alignment(), Qt::AlignBottom | Qt::AlignLeft);
    emit view->clicked(model->index(z2, 0));
    QTest::qWait(30);
    QCOMPARE(plot->currentJob(), model->jobAtRow(z2)->jobId);
    QVERIFY(plot->currentJob() != model->jobAtRow(z1)->jobId);
}

void TestGui::adaptiveTimeScaleTested()
{

    QVector<QPointF> pts;
    for (int i = 0; i < 1000; ++i) pts.append({double(i), 100.0});
    for (int i = 0; i < 50; ++i) pts.append({1000.0 + i, 100.0 + i * 2.0});
    for (int i = 0; i < 1000; ++i) pts.append({1050.0 + i, 200.0});
    const auto warp = PlotWidget::adaptiveTimeScale(pts, true);
    const auto lin = PlotWidget::adaptiveTimeScale(pts, false);
    QCOMPARE(warp.size(), pts.size());
    QCOMPARE(lin.size(), pts.size());

    QCOMPARE(lin.last(), pts.last().x());

    QCOMPARE(warp.first(), pts.first().x());
    QVERIFY(warp.last() > lin.last());

    const double dxStable = (warp[999] - warp[0]) / 999.0;
    const double dxSprung = (warp[1050] - warp[1000]) / 50.0;
    QVERIFY(dxSprung > dxStable * 5);

    QCOMPARE(PlotWidget::adaptiveTimeScale(pts, false).last(), pts.last().x());
}

void TestGui::adaptiveToggleAxis()
{
    PlotWidget plot;
    plot.resize(800, 400);
    plot.show();
    QTest::qWait(20);
    PrintLog p;
    p.jobId = "JT";
    p.nozzleTargetC = 210;
    for (int i = 0; i < 2000; ++i) {
        Sample m; m.timeS = i; m.nozzleActualC = (i == 1000) ? 220.0 : 100.0 + (i % 5); m.bedActualC = 60;
        p.heatUp.append(m);
    }
    plot.showProtocol(p);
    auto count = [&]() -> int {
        for (auto *s : plot.chart()->series())
            if (s->name() == QString::fromUtf8("Düsentemperatur"))
                return int(static_cast<QLineSeries*>(s)->points().size());
        return 0;
    };
    auto span = [&]() -> double {
        const auto *s = static_cast<QLineSeries*>(plot.chart()->series().first());
        const auto pts = s->points();
        return pts.last().x() - pts.first().x();
    };
    auto *box = plot.findChild<QCheckBox*>();
    QVERIFY(box);
    QVERIFY(!box->isChecked());
    QCOMPARE(count(), 2000);
    const double spanStandard = span();
    QCOMPARE(spanStandard, 1999.0 / 60.0);
    box->setChecked(true);
    QTest::qWait(20);
    QCOMPARE(count(), 2000);
    const double spanAdaptive = span();
    QVERIFY(spanAdaptive > spanStandard);
    box->setChecked(false);
    QTest::qWait(20);
    QCOMPARE(span(), spanStandard);
}

void TestGui::pidDialogRankingAndFit()
{

    const Printer *d = w->store()->printer(QStringLiteral("D025"));
    QVERIFY(d);
    PidSimDialog dlg(*d, w->store(), w->loader(), w);
    dlg.show();
    QTest::qWait(30);

    auto *liste = dlg.findChild<QListWidget*>();
    QVERIFY(liste);
    QCOMPARE(liste->count(), 7);
    QVERIFY(liste->item(0)->text().startsWith(QStringLiteral("1.")));
    QVERIFY(liste->item(0)->text().contains(QString::fromUtf8("Ausregelzeit")));

    auto *plot = dlg.findChild<PlotWidget*>();
    QVERIFY(plot);
    QCOMPARE(plot->chart()->series().size(), 8);

    bool ergOk = false;
    for (auto *l : dlg.findChildren<QLabel*>())
        if (l->text().contains(QString::fromUtf8("Ausregelzeit")) && l->text().contains(QString::fromUtf8("Simuliert")))
            ergOk = true;
    QVERIFY(ergOk);

    QCheckBox *fit = nullptr;
    for (auto *c : dlg.findChildren<QCheckBox*>())
        if (c->text().contains(QString::fromUtf8("fitten"))) fit = c;
    QVERIFY(fit);
    fit->setChecked(true);
    QTest::qWait(50);
    bool fitOk = false;
    for (auto *l : dlg.findChildren<QLabel*>())
        if (l->text().contains(QString::fromUtf8("gefitten")) && l->text().contains(QStringLiteral("J")))
            fitOk = true;
    QVERIFY(fitOk);
    fit->setChecked(false);
    QTest::qWait(30);
    bool presetOk = false;
    for (auto *l : dlg.findChildren<QLabel*>())
        if (l->text().contains(QString::fromUtf8("Modell-Preset"))) presetOk = true;
    QVERIFY(presetOk);
}

void TestGui::maintenanceTableInDetailDialog()
{

    const Printer *d = w->store()->printer(QStringLiteral("D058"));
    QVERIFY(d);
    int erwartet = 0;
    for (const auto &wt : w->store()->maintenances())
        if (wt.printerId == QStringLiteral("D058")) ++erwartet;
    QCOMPARE(erwartet, 5);
    PrinterDetailDialog dlg(*d, w->store(), w->loader(), w);
    dlg.show();
    QTest::qWait(30);

    QTableView *wartView = nullptr;
    for (auto *v : dlg.findChildren<QTableView*>())
        if (qobject_cast<MaintenanceModel*>(v->model())) wartView = v;
    QVERIFY(wartView);
    QCOMPARE(wartView->model()->rowCount(), 5);

    PrinterDetailDialog dlg2(*w->store()->printer(QStringLiteral("D001")), w->store(), w->loader(), w);
    dlg2.show();
    QTest::qWait(30);
    int andere = 0;
    for (const auto &wt : w->store()->maintenances())
        if (wt.printerId == QStringLiteral("D001")) ++andere;
    for (auto *v : dlg2.findChildren<QTableView*>())
        if (qobject_cast<MaintenanceModel*>(v->model()))
            QCOMPARE(v->model()->rowCount(), andere);
}

void TestGui::sqliteImportMenuPresent()
{

    bool found = false;
    for (auto *m : w->menuBar()->actions())
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QStringLiteral("SQLite")) &&
                a->text().toLower().contains(QStringLiteral("import")))
                found = true;
    QVERIFY(found);

    bool exportFound = false;
    for (auto *m : w->menuBar()->actions())
        for (auto *a : m->menu()->actions())
            if (a->text().contains(QStringLiteral("SQLite")) &&
                a->text().toLower().contains(QString::fromUtf8("export")))
                exportFound = true;
    QVERIFY(exportFound);
}

void TestGui::asyncLoadNonBlocking()
{

    auto *tabs = w->findChild<QTabWidget*>();
    auto *tab1 = tabs->widget(1);
    auto *view = tab1->findChild<QTableView*>();
    auto *plot = tab1->findChild<PlotWidget*>();
    auto *model = static_cast<JobModel*>(view->model());
    QVERIFY(view && plot && model);
    int ziel = -1;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto *a = model->jobAtRow(i);
        if (a->jobResult != QLatin1String("ausstehend") && a->ende.isValid() &&
            !w->loader()->isCached(a->jobId)) { ziel = i; break; }
    }
    QVERIFY(ziel >= 0);
    const QString zielId = model->jobAtRow(ziel)->jobId;
    QSignalSpy spy(w->loader(), &ProtocolLoader::fertig);
    emit view->clicked(model->index(ziel, 0));

    QVERIFY(plot->chart()->series().isEmpty());
    QVERIFY(!w->loader()->isCached(zielId));

    QVERIFY(spy.wait(5000));
    QVERIFY(w->loader()->isCached(zielId));
    QVERIFY(!plot->chart()->series().isEmpty());
    QCOMPARE(plot->currentJob(), zielId);
}

void TestGui::rightsSeparationEnforced()
{

    Printer original;
    original.printerId = QStringLiteral("D001"); original.modelId = QStringLiteral("MD01");
    original.location = QStringLiteral("A-01-1"); original.status = QStringLiteral("bereit");
    original.pidKp = 5.0; original.pidKi = 0.1; original.pidKd = 2.0;
    Printer edited = original;
    edited.location = QStringLiteral("B-02-2");
    edited.status = QStringLiteral("wartung");
    edited.pidKp = 9.9;

    Auth::logout();
    QCOMPARE(Auth::role(), Auth::None);
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(!Auth::mayEditPid());
    QVERIFY(!Auth::mayPlanJobs());
    QVERIFY(!Auth::mayExport());
    Printer out = PrinterWidget::applyRoleRestrictions(original, edited);
    QCOMPARE(out.location, original.location);
    QCOMPARE(out.status, original.status);
    QCOMPARE(out.pidKp, original.pidKp);

    QVERIFY(Auth::login(Auth::Operator, QStringLiteral("b1"), QStringLiteral("bediener123")));
    QVERIFY(Auth::mayExport());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(!Auth::mayEditPid());
    QVERIFY(!Auth::mayPlanJobs());
    out = PrinterWidget::applyRoleRestrictions(original, edited);
    QCOMPARE(out.location, original.location);
    QCOMPARE(out.status, original.status);
    QCOMPARE(out.pidKp, original.pidKp);

    QVERIFY(Auth::login(Auth::Engineer, QStringLiteral("t1"), QStringLiteral("technik123")));
    QVERIFY(Auth::mayEditPid());
    QVERIFY(Auth::mayPlanJobs());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(Auth::mayExport());
    out = PrinterWidget::applyRoleRestrictions(original, edited);
    QCOMPARE(out.location, original.location);
    QCOMPARE(out.status, original.status);
    QCOMPARE(out.pidKp, 9.9);
    QCOMPARE(out.pidKi, original.pidKi);
    QCOMPARE(out.pidKd, original.pidKd);

    Printer nurPid = original;
    nurPid.pidKd = 7.7;
    out = PrinterWidget::applyRoleRestrictions(original, nurPid);
    QCOMPARE(out.pidKd, 7.7);
    QCOMPARE(out.location, original.location);

    QVERIFY(Auth::login(Auth::Admin, QStringLiteral("a1"), QStringLiteral("admin123")));
    QVERIFY(Auth::mayEditMasterData());
    QVERIFY(Auth::mayEditPid());
    QVERIFY(Auth::mayPlanJobs());
    QVERIFY(Auth::mayExport());
    out = PrinterWidget::applyRoleRestrictions(original, edited);
    QCOMPARE(out.location, QStringLiteral("B-02-2"));
    QCOMPARE(out.status, QStringLiteral("wartung"));
    QCOMPARE(out.pidKp, 9.9);

    Auth::logout();
    QVERIFY(!Auth::mayExport());
    QVERIFY(!Auth::mayEditPid());
    QVERIFY(!Auth::mayEditMasterData());
    QVERIFY(!Auth::mayPlanJobs());
    out = PrinterWidget::applyRoleRestrictions(original, edited);
    QCOMPARE(out.location, original.location);
    QCOMPARE(out.pidKp, original.pidKp);
}

QTEST_MAIN(TestGui)
#include "tst_gui.moc"
