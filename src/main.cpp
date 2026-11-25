// Programmeinstieg: Datenordner finden/laden, Hauptfenster oeffnen.

#include <QApplication>
#include <QStatusBar>
#include <QLocale>
#include "mainwindow.h"
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PrintFarm_Gruppe_03"));
    QApplication::setOrganizationName(QStringLiteral("Gruppe_03"));

    QLocale::setDefault(QLocale::c());

    MainWindow w;

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList datenMarker = {
        QStringLiteral("data/drucker.csv"),
        QStringLiteral("QT-Projekt_info/PrintFarm_Daten/PrintFarm_Daten/drucker.csv")
    };
    QString found;
    for (const QString &start : {appDir, QDir::currentPath()}) {
        QDir d(start);
        for (int i = 0; i < 6 && found.isEmpty(); ++i) {
            for (const auto &m : datenMarker) {
                if (QFileInfo::exists(d.absoluteFilePath(m))) {
                    found = d.absoluteFilePath(m);
                    found.chop(QStringLiteral("drucker.csv").size());
                    break;
                }
            }
            if (!found.isEmpty() || !d.cdUp()) break;
        }
        if (!found.isEmpty()) break;
    }
    bool loaded = false;
    if (!found.isEmpty()) {
        qDebug() << "Datenordner gefunden:" << found;
        loaded = w.loadDataFolder(found);
    }
    if (!loaded) {
        w.statusBar()->showMessage(
            QObject::tr("Kein Datenordner gefunden - bitte über Datei > Datenordner wählen angeben."));
    }

    w.show();
    return app.exec();
}
