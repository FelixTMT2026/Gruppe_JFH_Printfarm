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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("PrintFarm — Monitoring der 3D-Druckerfarm"));
    resize(1200, 800);

    m_store = new DataStore(this);
    m_loader = new ProtocolLoader(this);

    m_status = new QLabel(this);
    statusBar()->addPermanentWidget(m_status);
    statusBar()->showMessage(tr("Bereit"));

    bauUI();
}

void MainWindow::bauUI()
{
    menuBar()->clear();
    auto *central = takeCentralWidget();
    delete central;

    m_tabs = new QTabWidget(this);
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

void MainWindow::themeUmschalten()
{
    m_dark = m_themeAction->isChecked();
    if (m_dark) {
        QPalette p;
        p.setColor(QPalette::Window, QColor(53, 53, 53));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(35, 35, 35));
        p.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
        qApp->setPalette(p);
        qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    } else {
        qApp->setStyle(QStyleFactory::create(QStringLiteral("windowsvista")));
        qApp->setPalette(qApp->style()->standardPalette());
    }
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
                QStringLiteral("Übersetzung app_en.qm nicht gefunden — Deutsch bleibt aktiv"));
            delete m_translator;
            m_translator = nullptr;
            m_english = false;
        }
    }
    bauUI();   // alle Texte werden mit tr() neu aufgebaut
    statusBar()->showMessage(m_english ? QStringLiteral("Language: English")
                                        : QStringLiteral("Sprache: Deutsch"));
}
