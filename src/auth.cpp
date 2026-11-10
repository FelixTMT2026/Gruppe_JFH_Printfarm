#include "auth.h"
#include <QCryptographicHash>
#include <QProcessEnvironment>

// Passwort-Hashes: SHA-256(Salt + Passwort). Default-Passwoerter (unbedingt
// aendern!): bediener123 / technik123 / admin123. Ueberschreibbar per
// Umgebungsvariable PRINTFARM_PW_BEDIENER / PRINTFARM_PW_TECHNIK / PRINTFARM_PW_ADMIN.
static const char *kSalt = "JFH-PrintFarm-v1";
static const char *kHashBediener = "d92f90378bf92efeed4d03e530e2157e23e0efad519614e2cdb67dd2979513e8";
static const char *kHashTechnik  = "c3809b75e5f9e215d16c5f79f7a1136a7e7e019a7f5a8143bf0fc1b1fe5bed52";
static const char *kHashAdmin    = "f6442b91a50d78f8d8433ed4c82b8982bade09f07b912ca79f4623dfea0f06c8";

static QString hashPasswort(const QString &pw)
{
    const QByteArray data = QByteArray(kSalt) + pw.toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

static QString hashFuerRolle(Auth::Rolle r)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    switch (r) {
    case Auth::Bediener: {
        const QString overridePw = env.value(QStringLiteral("PRINTFARM_PW_BEDIENER"));
        return overridePw.isEmpty() ? QString::fromLatin1(kHashBediener) : hashPasswort(overridePw);
    }
    case Auth::Technik: {
        const QString overridePw = env.value(QStringLiteral("PRINTFARM_PW_TECHNIK"));
        return overridePw.isEmpty() ? QString::fromLatin1(kHashTechnik) : hashPasswort(overridePw);
    }
    case Auth::Admin: {
        const QString overridePw = env.value(QStringLiteral("PRINTFARM_PW_ADMIN"));
        return overridePw.isEmpty() ? QString::fromLatin1(kHashAdmin) : hashPasswort(overridePw);
    }
    default: return {};
    }
}

static Auth::Rolle g_rolle = Auth::Keine;
static QString g_name;

// Vergleich in konstanter Zeit, um Timing-Angriffe zu erschweren
static bool constantTimeCompare(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}

bool Auth::passwortOk(Rolle r, const QString &passwort)
{
    if (r == Keine) return false;
    const QByteArray a = hashPasswort(passwort).toLatin1();
    const QByteArray b = hashFuerRolle(r).toLatin1();
    return constantTimeCompare(a, b);
}

bool Auth::login(Rolle r, const QString &name, const QString &passwort)
{
    if (r == Keine || !passwortOk(r, passwort))
        return false;
    g_rolle = r;
    g_name = name;
    return true;
}
void Auth::logout() { g_rolle = Keine; g_name.clear(); }
Auth::Rolle Auth::rolle() { return g_rolle; }
QString Auth::benutzerName() { return g_name; }

bool Auth::darfStammdatenAendern() { return g_rolle == Admin; }
bool Auth::darfPidAendern() { return g_rolle == Technik || g_rolle == Admin; }
bool Auth::darfAuftraegePlanen() { return g_rolle == Technik || g_rolle == Admin; }
bool Auth::darfExportieren() { return g_rolle != Keine; }
