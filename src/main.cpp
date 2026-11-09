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

    // Datenordner relativ zur Anwendung auflösen (offline, relative Pfade).
    // Vom Exe-Pfad und vom Arbeitsverzeichnis jeweils nach oben suchen, damit
    // der Start auch über eine Verknüpfung aus einem beliebigen Verzeichnis klappt.
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList datenMarker = {
        QStringLiteral("data/drucker.csv"),
        QStringLiteral("QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten/drucker.csv")
    };
    QString gefunden;
    for (const QString &start : {appDir, QDir::currentPath()}) {
        QDir d(start);
        for (int i = 0; i < 6 && gefunden.isEmpty(); ++i) {
            for (const auto &m : datenMarker) {
                if (QFileInfo::exists(d.absoluteFilePath(m))) {
                    gefunden = d.absoluteFilePath(m);
                    gefunden.chop(QStringLiteral("drucker.csv").size());
                    break;
                }
            }
            if (!gefunden.isEmpty() || !d.cdUp()) break;
        }
        if (!gefunden.isEmpty()) break;
    }
    bool geladen = false;
    if (!gefunden.isEmpty()) {
        qDebug() << "Datenordner gefunden:" << gefunden;
        geladen = w.ladeDatenordner(gefunden);
    }
    if (!geladen) {
        w.statusBar()->showMessage(
            QObject::tr("Kein Datenordner gefunden — bitte über Datei > Datenordner wählen angeben."));
    }

    w.show();
    return app.exec();
}
