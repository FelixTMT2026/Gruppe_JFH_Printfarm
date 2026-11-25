// Passwort-Hashes (SHA-256 + Salt), Sitzung, Rechtepruefungen - siehe auth.h.

#include "auth.h"
#include <QCryptographicHash>
#include <QProcessEnvironment>

static const char *passwordSalt = "JFH-PrintFarm-v1";
static const char *hashBedienerVorgabe = "d92f90378bf92efeed4d03e530e2157e23e0efad519614e2cdb67dd2979513e8";
static const char *hashTechnikVorgabe  = "c3809b75e5f9e215d16c5f79f7a1136a7e7e019a7f5a8143bf0fc1b1fe5bed52";
static const char *hashAdminVorgabe    = "f6442b91a50d78f8d8433ed4c82b8982bade09f07b912ca79f4623dfea0f06c8";

static QString hashPassword(const QString &pw)
{
    const QByteArray data = QByteArray(passwordSalt) + pw.toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

static QString expectedHashForRole(Auth::Role r)
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    switch (r) {
    case Auth::Operator: {
        const QString overridePw = env.value(QStringLiteral("PRINTFARM_PW_BEDIENER"));
        return overridePw.isEmpty() ? QString::fromLatin1(hashBedienerVorgabe) : hashPassword(overridePw);
    }
    case Auth::Engineer: {
        const QString overridePw = env.value(QStringLiteral("PRINTFARM_PW_TECHNIK"));
        return overridePw.isEmpty() ? QString::fromLatin1(hashTechnikVorgabe) : hashPassword(overridePw);
    }
    case Auth::Admin: {
        const QString overridePw = env.value(QStringLiteral("PRINTFARM_PW_ADMIN"));
        return overridePw.isEmpty() ? QString::fromLatin1(hashAdminVorgabe) : hashPassword(overridePw);
    }
    default: return {};
    }
}

Auth::Session &Auth::session()
{
    static Session s;
    return s;
}

static bool constantTimeCompare(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}

// SHA-256 mit statischem Salt; Default-Passwoerter liegen im Quelltext (Hochschulprojekt,
// Doku siehe README) und sind per Umgebungsvariable PRINTFARM_PW_* ueberschreibbar.
bool Auth::passwordOk(Role r, const QString &password)
{
    if (r == None) return false;
    const QByteArray a = hashPassword(password).toLatin1();
    const QByteArray b = expectedHashForRole(r).toLatin1();
    return constantTimeCompare(a, b);
}

bool Auth::login(Role r, const QString &name, const QString &password)
{
    if (r == None || !passwordOk(r, password))
        return false;
    session().role = r;
    session().userName = name;
    return true;
}
void Auth::logout() { session() = Session{}; }
Auth::Role Auth::role() { return session().role; }
QString Auth::userName() { return session().userName; }

bool Auth::mayEditMasterData() { return session().role == Admin; }
bool Auth::mayEditPid() { return session().role == Engineer || session().role == Admin; }
bool Auth::mayPlanJobs() { return session().role == Engineer || session().role == Admin; }
bool Auth::mayExport() { return session().role != None; }
