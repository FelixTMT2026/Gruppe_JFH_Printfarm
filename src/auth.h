#pragma once
// Einfaches Rollen-/Rechtekonzept (Erweiterung 3): bediener, technik, admin.
// Rechteprüfungen zentral hier, damit sie auch außerhalb der GUI greifen.

#include <QString>

class Auth {
public:
    enum Rolle { Keine = 0, Bediener, Technik, Admin };

    static void login(Rolle r, const QString &name);
    static void logout();
    static Rolle rolle();
    static QString benutzerName();

    // zentrale Rechteprüfung
    static bool darfStammdatenAendern();      // admin
    static bool darfPidAendern();             // technik + admin
    static bool darfAuftraegePlanen();        // technik + admin
    static bool darfExportieren();            // alle angemeldeten
};
