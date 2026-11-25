// Implementierung siehe mainwindow.h.

#include "mainwindow.h"
#include "theme.h"
#include "druckerwidget.h"
#include "teilewidget.h"
#include "materialwidget.h"
#include "fehlerlogwidget.h"
#include "dialogs.h"
#include "plotwidget.h"
#include "auth.h"
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QFrame>
#include <QChart>
#include <QChartView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QKeyEvent>
#include <QStyleHints>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("PrintFarm - Monitoring der 3D-Druckerfarm"));

    const QRect screenGeo = QGuiApplication::primaryScreen()->availableGeometry();
    setMinimumSize(qMax(480, screenGeo.width() / 2),
                   qMax(320, screenGeo.height() / 2));
    resize(qMin(1280, screenGeo.width()),
           qMin(840, screenGeo.height()));

    m_store = new DataStore(this);
    m_loader = new ProtocolLoader(this);

    m_dark = systemPrefersDarkMode();

    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);
    statusBar()->showMessage(tr("Bereit"));

    applyTheme();
    buildUi();
}

bool MainWindow::systemPrefersDarkMode()
{

    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark)
        return true;
    if (scheme == Qt::ColorScheme::Light)
        return false;

    const QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
                        QSettings::NativeFormat);
    const QVariant v = reg.value(QStringLiteral("AppsUseLightTheme"));
    if (v.isValid())
        return v.toInt() == 0;

    return true;
}

void MainWindow::buildUi()
{
    menuBar()->clear();
    if (m_tabs)
        m_activeTab = m_tabs->currentIndex();
    auto *central = takeCentralWidget();
    delete central;

    m_tabs = new QTabWidget(this);
    m_tabs->setTabPosition(QTabWidget::North);
    m_tabs->tabBar()->setExpanding(true);
    m_tabs->setDocumentMode(true);
    auto *printerTab = new PrinterWidget(m_store, m_loader, this);
    auto *partsTab = new PartsWidget(m_store, m_loader, this);
    auto *materialTab = new MaterialWidget(m_store, this);
    auto *errorTab = new ErrorLogWidget(m_store, this);
    m_tabs->addTab(printerTab, tr("Druckerliste"));
    m_tabs->addTab(partsTab, tr("Gedruckte Teile"));
    m_tabs->addTab(materialTab, tr("Material"));
    m_tabs->addTab(errorTab, tr("Fehlerlog"));

    setCentralWidget(m_tabs);
    m_tabs->setCurrentIndex(m_activeTab);

    connect(printerTab, &PrinterWidget::statusMessage, this, [this](const QString &t) {
        m_status->setText(t);
    });
    connect(partsTab, &PartsWidget::statusMessage, this, [this](const QString &t) {
        m_status->setText(t);
    });

    buildMenus();
}

void MainWindow::buildMenus()
{
    auto *datei = menuBar()->addMenu(tr("&Datei"));
    auto *folderAction = datei->addAction(tr("Datenordner wählen..."));
    connect(folderAction, &QAction::triggered, this, &MainWindow::chooseDataFolder);
    auto *aCsv = datei->addAction(tr("CSV exportieren (drucker_out/auftraege_out)..."));
    connect(aCsv, &QAction::triggered, this, &MainWindow::runCsvExport);
    auto *aSql = datei->addAction(tr("SQLite exportieren..."));
    connect(aSql, &QAction::triggered, this, &MainWindow::runSqliteExport);
    auto *aSqlIn = datei->addAction(tr("SQLite importieren..."));
    connect(aSqlIn, &QAction::triggered, this, &MainWindow::runSqliteImport);
    auto *reportAction = datei->addAction(tr("Bericht exportieren..."));
    connect(reportAction, &QAction::triggered, this, &MainWindow::runReportExport);
    datei->addSeparator();
    auto *aBeenden = datei->addAction(tr("Beenden"));
    connect(aBeenden, &QAction::triggered, qApp, &QApplication::quit);

    auto *ansicht = menuBar()->addMenu(tr("&Ansicht"));
    m_themeAction = ansicht->addAction(tr("Darkmode umschalten"));
    m_themeAction->setCheckable(true);
    m_themeAction->setChecked(m_dark);
    connect(m_themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);
    m_fsAction = ansicht->addAction(tr("Vollbild (F)"));
    m_fsAction->setCheckable(true);
    connect(m_fsAction, &QAction::triggered, this, [this]() { toggleFullscreen(); });
    m_langAction = ansicht->addAction(m_english ? tr("Zu Deutsch wechseln") : tr("Switch to English"));
    connect(m_langAction, &QAction::triggered, this, &MainWindow::switchLanguage);

    auto *loginMenu = menuBar()->addMenu(tr("&Anmeldung"));
    m_loginAction = loginMenu->addAction(
        Auth::role() == Auth::None ? tr("Anmelden...")
        : tr("Abmelden (%1)").arg(Auth::userName()));
    connect(m_loginAction, &QAction::triggered, this, &MainWindow::login);
}

bool MainWindow::loadDataFolder(const QString &folder)
{
    m_loader->loadIndex(folder);
    if (!m_store->loadFromFolder(folder))
        return false;
    statusBar()->showMessage(tr("Daten geladen aus %1 (%2 Drucker, %3 Aufträge)")
        .arg(folder).arg(m_store->printer().size()).arg(m_store->auftraege().size()));
    return true;
}

void MainWindow::chooseDataFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Datenordner wählen"),
                                                          m_store->dataFolder());
    if (dir.isEmpty()) return;
    loadDataFolder(dir);
}

void MainWindow::runCsvExport()
{
    if (!Auth::mayExport()) {
        m_store->log(LogEntry::System, tr("Export verweigert (nicht angemeldet): CSV-Export"));
        QMessageBox::information(this, tr("Rechte"), tr("Bitte zuerst anmelden."));
        return;
    }
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Zielordner für CSV-Export"));
    if (dir.isEmpty()) return;
    QString err;
    if (m_store->exportCsv(dir, &err))
        statusBar()->showMessage(tr("CSV-Export erfolgreich nach %1").arg(dir));
    else
        QMessageBox::warning(this, tr("Export"), err);
}

void MainWindow::runSqliteExport()
{

    if (!Auth::mayExport()) {
        m_store->log(LogEntry::System, tr("Export verweigert (nicht angemeldet): SQLite-Export"));
        QMessageBox::information(this, tr("Rechte"), tr("Bitte zuerst anmelden."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("SQLite-Datei speichern"),
                                                      QStringLiteral("PrintFarm_daten_out.sqlite"),
                                                      tr("SQLite (*.sqlite)"));
    if (path.isEmpty()) return;
    QString err;
    if (m_store->exportSqlite(path, &err))
        statusBar()->showMessage(tr("SQLite-Export erfolgreich: %1").arg(path));
    else
        QMessageBox::warning(this, tr("Export"), err);
}

void MainWindow::runSqliteImport()
{

    if (!Auth::mayExport()) {
        QMessageBox::information(this, tr("Rechte"), tr("Bitte zuerst anmelden."));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("SQLite-Datei importieren"),
                                                      m_store->dataFolder(),
                                                      tr("SQLite (*.sqlite *.db)"));
    if (path.isEmpty()) return;
    if (m_store->importSqlite(path))
        statusBar()->showMessage(tr("SQLite-Import erfolgreich: %1").arg(path));
    else
        QMessageBox::warning(this, tr("Import"),
                             tr("SQLite-Import fehlgeschlagen (siehe Fehlerlog)."));
}

void MainWindow::runReportExport()
{

    if (!Auth::mayExport()) {
        m_store->log(LogEntry::System, tr("Export verweigert (nicht angemeldet): Bericht-Export"));
        QMessageBox::information(this, tr("Rechte"), tr("Bitte zuerst anmelden."));
        return;
    }
    ReportDialog dlg(m_store, this);
    dlg.exec();
}

void MainWindow::login()
{
    if (Auth::role() != Auth::None) {
        Auth::logout();
        m_store->log(LogEntry::System, tr("Abmeldung: %1").arg(Auth::userName()));
        buildUi();
        return;
    }
    LoginDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (!Auth::login(dlg.selectedRole(), dlg.name(), dlg.password())) {
            m_store->log(LogEntry::System, tr("Fehlgeschlagene Anmeldung (falsches Passwort)"));
            QMessageBox::warning(this, tr("Anmeldung"),
                tr("Falsches Passwort für die gewählte Rolle."));
            statusBar()->showMessage(tr("Anmeldung fehlgeschlagen: falsches Passwort."));
            return;
        }
        m_store->log(LogEntry::System,
            tr("Anmeldung: %1 als %2").arg(Auth::userName(),
                Auth::role() == Auth::Admin ? QStringLiteral("admin") :
                Auth::role() == Auth::Engineer ? QStringLiteral("technik") : QStringLiteral("bediener")));
        buildUi();
    }
}

void MainWindow::applyTheme()
{

    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    qApp->setStyleSheet(m_dark ? dunklesStylesheet() : hellesStylesheet());
    qApp->setPalette(m_dark ? dunklePalette() : hellePalette());

    const auto charts = findChildren<QChartView*>();
    for (auto *c : charts)
        c->chart()->setTheme(m_dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
}

void MainWindow::toggleTheme()
{
    m_dark = m_themeAction->isChecked();
    applyTheme();
}

void MainWindow::toggleFullscreen()
{

    if (isMaximized())
        showNormal();
    else
        showMaximized();
    if (m_fsAction)
        m_fsAction->setChecked(isMaximized());
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{

    if (e->key() == Qt::Key_F && e->modifiers() == Qt::NoModifier) {
        toggleFullscreen();
        e->accept();
        return;
    }
    QMainWindow::keyPressEvent(e);
}

void MainWindow::switchLanguage()
{
    m_english = !m_english;
    if (m_translator) {
        qApp->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    if (m_english) {
        m_translator = new QTranslator(qApp);
        const QString qm = QCoreApplication::applicationDirPath() + QStringLiteral("/app_en.qm");
        if (m_translator->load(qm))
            qApp->installTranslator(m_translator);
        else {
            m_store->log(LogEntry::File,
                QStringLiteral("Übersetzung app_en.qm nicht gefunden - Deutsch bleibt aktiv"));
            delete m_translator;
            m_translator = nullptr;
            m_english = false;
        }
    }
    buildUi();
    statusBar()->showMessage(m_english ? QStringLiteral("Language: English")
                                        : QStringLiteral("Sprache: Deutsch"));
}
