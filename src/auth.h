#pragma once
// Einfaches Rollen-/Rechtekonzept (Erweiterung 3): bediener, technik, admin.
// Rechteprüfungen zentral hier, damit sie auch außerhalb der GUI greifen.

#include <QString>

class Auth {
public:
    enum Rolle { Keine = 0, Bediener, Technik, Admin };

    // Login mit Passwort-Pruefung (SHA-256, siehe auth.cpp). Liefert false,
    // wenn das Passwort zur Rolle nicht stimmt - dann bleibt abgemeldet.
    static bool login(Rolle r, const QString &name, const QString &passwort);
    static void logout();
    // Passwort-Pruefung einzeln (z. B. fuer Tests)
    static bool passwortOk(Rolle r, const QString &passwort);
    static Rolle rolle();
    static QString benutzerName();

    // zentrale Rechteprüfung
    static bool darfStammdatenAendern();      // admin
    static bool darfPidAendern();             // technik + admin
    static bool darfAuftraegePlanen();        // technik + admin
    static bool darfExportieren();            // alle angemeldeten
};
