// Hauptfenster: vier Tabs, Menues, Light/Darkmode, Sprachumschaltung, Login.
#pragma once

#include "datastore.h"
#include "protocol.h"
#include <QMainWindow>
#include <QTranslator>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool loadDataFolder(const QString &folder);
    DataStore *store() const { return m_store; }
    ProtocolLoader *loader() const { return m_loader; }
protected:
    void keyPressEvent(class QKeyEvent *e) override;
private:
    void buildUi();
    void buildMenus();
    void applyTheme();
    static bool systemPrefersDarkMode();
    void toggleTheme();
    void switchLanguage();
    void login();
    void chooseDataFolder();
    void runCsvExport();
    void runSqliteExport();
    void runSqliteImport();
    void runReportExport();
    void toggleFullscreen();

    DataStore *m_store;
    ProtocolLoader *m_loader;
    class QTabWidget *m_tabs = nullptr;
    class QLabel *m_status;
    class QAction *m_themeAction;
    class QAction *m_fsAction;
    class QAction *m_langAction;
    class QAction *m_loginAction;
    QTranslator *m_translator = nullptr;
    bool m_dark = false;
    bool m_english = false;
    int m_activeTab = 0;
};
