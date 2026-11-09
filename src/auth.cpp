#include "auth.h"

static Auth::Rolle g_rolle = Auth::Keine;
static QString g_name;

void Auth::login(Rolle r, const QString &name) { g_rolle = r; g_name = name; }
void Auth::logout() { g_rolle = Keine; g_name.clear(); }
Auth::Rolle Auth::rolle() { return g_rolle; }
QString Auth::benutzerName() { return g_name; }

bool Auth::darfStammdatenAendern() { return g_rolle == Admin; }
bool Auth::darfPidAendern() { return g_rolle == Technik || g_rolle == Admin; }
bool Auth::darfAuftraegePlanen() { return g_rolle == Technik || g_rolle == Admin; }
bool Auth::darfExportieren() { return g_rolle != Keine; }
