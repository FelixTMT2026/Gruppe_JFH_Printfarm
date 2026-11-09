#pragma once
// Hauptfenster: genau vier Haupttabs in der vorgeschriebenen Reihenfolge,
// Menü mit Datenordner-Wahl, Export, Login, Theme (Light/Dark) und Sprache.

#include "datastore.h"
#include "protocol.h"
#include <QMainWindow>
#include <QTranslator>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool ladeDatenordner(const QString &folder);
private:
    void bauUI();
    void menuesErzeugen();
    void themeUmschalten();
    void spracheUmschalten();
    void login();
    void datenordnerWaehlen();
    void csvExportieren();
    void sqliteExportieren();
    void berichtExportieren();

    DataStore *m_store;
    ProtocolLoader *m_loader;
    class QTabWidget *m_tabs;
    class QLabel *m_status;
    class QAction *m_themeAction;
    class QAction *m_langAction;
    class QAction *m_loginAction;
    QTranslator *m_translator = nullptr;
    bool m_dark = false;
    bool m_english = false;
};
