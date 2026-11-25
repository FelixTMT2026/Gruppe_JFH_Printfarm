// Rollen-/Rechtekonzept (bediener/technik/admin) mit zentraler Sitzung.
#pragma once

#include <QString>

class Auth {
public:
    enum Role { None = 0, Operator, Engineer, Admin };

    struct Session {
        Role role = None;
        QString userName;
    };

    static bool login(Role r, const QString &name, const QString &password);
    static void logout();

    static bool passwordOk(Role r, const QString &password);
    static Role role();
    static QString userName();

    static bool mayEditMasterData();
    static bool mayEditPid();
    static bool mayPlanJobs();
    static bool mayExport();

private:
    static Session &session();
};
