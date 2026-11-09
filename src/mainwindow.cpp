#include "mainwindow.h"
#include "druckerwidget.h"
#include "teilewidget.h"
#include "materialwidget.h"
#include "fehlerlogwidget.h"
#include "dialogs.h"
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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QGuiApplication>
#include <QStyleHints>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("PrintFarm - Monitoring der 3D-Druckerfarm"));
    setMinimumSize(960, 640);
    resize(1280, 840);

    m_store = new DataStore(this);
    m_loader = new ProtocolLoader(this);

    // System-Default: Windows-Theme (hell/dunkel) übernehmen
    m_dark = systemIstDunkel();

    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);
    statusBar()->showMessage(tr("Bereit"));

    applyTheme();   // vor dem UI-Aufbau, damit z. B. Charts das richtige Theme sehen
    bauUI();
}

bool MainWindow::systemIstDunkel()
{
    // 1) Qt-Color-Scheme (Windows 11 / Qt 6.5+)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark)
        return true;
    if (scheme == Qt::ColorScheme::Light)
        return false;
    // 2) Fallback: Windows-Registry (AppsUseLightTheme = 0 -> dunkles Systemtheme)
    const QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
                        QSettings::NativeFormat);
    const QVariant v = reg.value(QStringLiteral("AppsUseLightTheme"), 1);
    return v.isValid() && v.toInt() == 0;
}

void MainWindow::bauUI()
{
    menuBar()->clear();
    auto *central = takeCentralWidget();
    delete central;

    m_tabs = new QTabWidget(this);
    m_tabs->setTabPosition(QTabWidget::North);            // Tabs oben
    m_tabs->tabBar()->setExpanding(true);                 // füllen die Breite gleichmäßig (zentriert)
    m_tabs->setDocumentMode(true);
    auto *druckerTab = new DruckerWidget(m_store, m_loader, this);
    auto *teileTab = new TeileWidget(m_store, m_loader, this);
    auto *materialTab = new MaterialWidget(m_store, this);
    auto *fehlerTab = new FehlerlogWidget(m_store, this);
    m_tabs->addTab(druckerTab, tr("Druckerliste"));
    m_tabs->addTab(teileTab, tr("Gedruckte Teile"));
    m_tabs->addTab(materialTab, tr("Material"));
    m_tabs->addTab(fehlerTab, tr("Fehlerlog"));

    setCentralWidget(m_tabs);

    connect(druckerTab, &DruckerWidget::statusMeldung, this, [this](const QString &t) {
        m_status->setText(t);
    });
    connect(teileTab, &TeileWidget::statusMeldung, this, [this](const QString &t) {
        m_status->setText(t);
    });

    menuesErzeugen();
}

void MainWindow::menuesErzeugen()
{
    auto *datei = menuBar()->addMenu(tr("&Datei"));
    auto *aOrdner = datei->addAction(tr("Datenordner wählen..."));
    connect(aOrdner, &QAction::triggered, this, &MainWindow::datenordnerWaehlen);
    auto *aCsv = datei->addAction(tr("CSV exportieren (drucker_out/auftraege_out)..."));
    connect(aCsv, &QAction::triggered, this, &MainWindow::csvExportieren);
    auto *aSql = datei->addAction(tr("SQLite exportieren..."));
    connect(aSql, &QAction::triggered, this, &MainWindow::sqliteExportieren);
    auto *aBericht = datei->addAction(tr("Bericht exportieren..."));
    connect(aBericht, &QAction::triggered, this, &MainWindow::berichtExportieren);
    datei->addSeparator();
    auto *aBeenden = datei->addAction(tr("Beenden"));
    connect(aBeenden, &QAction::triggered, qApp, &QApplication::quit);

    auto *ansicht = menuBar()->addMenu(tr("&Ansicht"));
    m_themeAction = ansicht->addAction(tr("Darkmode umschalten"));
    m_themeAction->setCheckable(true);
    m_themeAction->setChecked(m_dark);
    connect(m_themeAction, &QAction::triggered, this, &MainWindow::themeUmschalten);
    m_langAction = ansicht->addAction(m_english ? tr("Zu Deutsch wechseln") : tr("Switch to English"));
    connect(m_langAction, &QAction::triggered, this, &MainWindow::spracheUmschalten);

    auto *loginMenu = menuBar()->addMenu(tr("&Anmeldung"));
    m_loginAction = loginMenu->addAction(
        Auth::rolle() == Auth::Keine ? tr("Anmelden...")
        : tr("Abmelden (%1)").arg(Auth::benutzerName()));
    connect(m_loginAction, &QAction::triggered, this, &MainWindow::login);
}

bool MainWindow::ladeDatenordner(const QString &folder)
{
    if (!m_store->loadFromFolder(folder))
        return false;
    m_loader->loadIndex(folder);
    statusBar()->showMessage(tr("Daten geladen aus %1 (%2 Drucker, %3 Aufträge)")
        .arg(folder).arg(m_store->drucker().size()).arg(m_store->auftraege().size()));
    return true;
}

void MainWindow::datenordnerWaehlen()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Datenordner wählen"),
                                                          m_store->dataFolder());
    if (dir.isEmpty()) return;
    ladeDatenordner(dir);
}

void MainWindow::csvExportieren()
{
    if (!Auth::darfExportieren()) {
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

void MainWindow::sqliteExportieren()
{
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

void MainWindow::berichtExportieren()
{
    BerichtDialog dlg(m_store, this);
    dlg.exec();
}

void MainWindow::login()
{
    if (Auth::rolle() != Auth::Keine) {
        Auth::logout();
        m_store->addLog(LogEintrag::System, tr("Abmeldung: %1").arg(Auth::benutzerName()));
        bauUI();
        return;
    }
    LoginDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        Auth::login(dlg.ausgewaehlteRolle(), dlg.name());
        m_store->addLog(LogEintrag::System,
            tr("Anmeldung: %1 als %2").arg(Auth::benutzerName(),
                Auth::rolle() == Auth::Admin ? QStringLiteral("admin") :
                Auth::rolle() == Auth::Technik ? QStringLiteral("technik") : QStringLiteral("bediener")));
        bauUI();
    }
}

void MainWindow::applyTheme()
{
    // Farbwelt aus den beiden Layout-Entwürfen (Lightmode/Darkmode):
    // Light: Header #B9C1CD, Sidebar #DAE0E8, Content #D9DFE7, Akzent #001065
    // Dark:  Header #212832, Sidebar #1F2D3B, Content #202B39, Akzent hellblau
    QString sheet;
    if (m_dark) {
        sheet = QStringLiteral(
            "QMainWindow, QDialog { background: #202B39; color: #E6ECF3; }"
            "QWidget { color: #E6ECF3; }"
            "QTabBar { }"
            "QMenuBar { background: #212832; color: #E6ECF3; }"
            "QMenuBar::item:selected { background: #31445A; }"
            "QMenu { background: #26333F; color: #E6ECF3; border: 1px solid #31445A; }"
            "QMenu::item:selected { background: #31445A; }"
            "QTabWidget::pane { border: none; background: #202B39; }"
            "QTabBar::tab { background: #1F2D3B; color: #AEB9C6; padding: 12px 22px; border: none; }"
            "QTabBar::tab:selected { background: #202B39; color: #6FA0FF; font-weight: bold; border-bottom: 3px solid #4A7DFF; }"
            "QTabBar::tab:hover { background: #26333F; }"
            "QTableView { background: #26333F; alternate-background-color: #2B3947; border: 1px solid #31445A; gridline-color: #31445A; color: #E6ECF3; selection-background-color: #4A7DFF; }"
            "QHeaderView::section { background: #212832; color: #E6ECF3; border: none; padding: 6px; font-weight: bold; }"
            "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #1C2630; border: 1px solid #31445A; color: #E6ECF3; padding: 5px; border-radius: 3px; }"
            "QPushButton { background: #4A7DFF; color: white; padding: 6px 16px; border: none; border-radius: 4px; }"
            "QPushButton:hover { background: #6FA0FF; }"
            "QPushButton:disabled { background: #31445A; color: #7C8A99; }"
            "QGroupBox { border: 1px solid #31445A; border-radius: 4px; margin-top: 14px; }"
            "QGroupBox::title { color: #6FA0FF; subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
            "QStatusBar { background: #1C2228; color: #AEB9C6; }"
            "QSplitter::handle { background: #31445A; }"
            "QLabel { color: #E6ECF3; }"
            "QToolTip { background: #26333F; color: #E6ECF3; border: 1px solid #31445A; }"
        );
    } else {
        sheet = QStringLiteral(
            "QMainWindow, QDialog { background: #D9DFE7; color: #1C2733; }"
            "QWidget { color: #1C2733; }"
            "QTabBar { }"
            "QMenuBar { background: #B9C1CD; color: #1C2733; }"
            "QMenuBar::item:selected { background: #9AA6B8; }"
            "QMenu { background: #FFFFFF; color: #1C2733; border: 1px solid #B9C1CD; }"
            "QMenu::item:selected { background: #D9DFE7; color: #001065; }"
            "QTabWidget::pane { border: none; background: #D9DFE7; }"
            "QTabBar::tab { background: #DAE0E8; color: #4A5568; padding: 12px 22px; border: none; }"
            "QTabBar::tab:selected { background: #D9DFE7; color: #001065; font-weight: bold; border-bottom: 3px solid #001065; }"
            "QTabBar::tab:hover { background: #CBD3DE; }"
            "QTableView { background: #FFFFFF; alternate-background-color: #EEF1F5; border: 1px solid #C4CCD8; gridline-color: #C4CCD8; color: #1C2733; selection-background-color: #001065; selection-color: #FFFFFF; }"
            "QHeaderView::section { background: #B9C1CD; color: #1C2733; border: none; padding: 6px; font-weight: bold; }"
            "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #FFFFFF; border: 1px solid #B9C1CD; color: #1C2733; padding: 5px; border-radius: 3px; }"
            "QPushButton { background: #001065; color: white; padding: 6px 16px; border: none; border-radius: 4px; }"
            "QPushButton:hover { background: #1A2A8C; }"
            "QPushButton:disabled { background: #9AA6B8; color: #D9DFE7; }"
            "QGroupBox { border: 1px solid #B9C1CD; border-radius: 4px; margin-top: 14px; }"
            "QGroupBox::title { color: #001065; subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
            "QStatusBar { background: #DDE2E7; color: #4A5568; }"
            "QSplitter::handle { background: #C4CCD8; }"
            "QToolTip { background: #FFFFFF; color: #1C2733; border: 1px solid #B9C1CD; }"
        );
    }
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    qApp->setStyleSheet(sheet);
}

void MainWindow::themeUmschalten()
{
    m_dark = m_themeAction->isChecked();
    applyTheme();
}

void MainWindow::spracheUmschalten()
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
            m_store->addLog(LogEintrag::Datei,
                QStringLiteral("Übersetzung app_en.qm nicht gefunden - Deutsch bleibt aktiv"));
            delete m_translator;
            m_translator = nullptr;
            m_english = false;
        }
    }
    bauUI();   // alle Texte werden mit tr() neu aufgebaut
    statusBar()->showMessage(m_english ? QStringLiteral("Language: English")
                                        : QStringLiteral("Sprache: Deutsch"));
}
