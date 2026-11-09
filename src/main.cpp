#include <QApplication>
#include <QStatusBar>
#include "mainwindow.h"
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Gruppe_JFH_Printfarm"));
    QApplication::setOrganizationName(QStringLiteral("JFH"));

    MainWindow w;

    // Datenordner relativ zur Anwendung auflösen (offline, relative Pfade):
    // Kandidaten: data/, QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten (Repo-Layout)
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList kandidaten = {
        appDir + "/data",
        appDir + "/QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten",
        QStringLiteral("data"),
        QStringLiteral("QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten")
    };
    bool geladen = false;
    for (const auto &k : kandidaten) {
        if (QFileInfo::exists(k + "/drucker.csv")) {
            geladen = w.ladeDatenordner(QDir(k).absolutePath());
            if (geladen) break;
        }
    }
    if (!geladen) {
        w.statusBar()->showMessage(
            QObject::tr("Kein Datenordner gefunden — bitte über Datei > Datenordner wählen angeben."));
    }

    w.show();
    return app.exec();
}
