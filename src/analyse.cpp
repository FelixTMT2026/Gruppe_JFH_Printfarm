#include "analyse.h"
#include "regelguete.h"
#include <QtMath>

Fehlererkennung::Befunde Fehlererkennung::analysiere(const Protokoll &p, const Regelguete &rg,
                                                      const Material *material)
{
    Befunde b;
    // Schlecht eingestellter Regler: großes Überschwingen (> 5 K), Dauerschwingung
    // (Amplitude am Ende der Aufheizphase > 2 K) oder sehr lange Ausregelzeit.
    if (rg.verfuegbar) {
        bool dauerschwingung = false;
        if (p.aufheizen.size() >= 20) {
            const int n = p.aufheizen.size();
            double amp = qAbs(p.aufheizen[n - 1].dueseIstC - p.aufheizen[n - 11].dueseIstC);
            dauerschwingung = amp > 2.0;
        }
        if (rg.ueberschwingenK > 5.0 || dauerschwingung || (!rg.ausgeregelt) || rg.ausregelzeitS > 300)
            b.schlechtEingestellt = true;
    }
    // Düse verstopft: Extruderstrom steigt in der Druckphase kontinuierlich an.
    if (p.druck.size() >= 20) {
        const int n = p.druck.size();
        const double anfang = p.druck[n / 4].extruderStromA;
        const double ende = p.druck[n - 1].extruderStromA;
        if (ende > anfang * 1.5 && ende - anfang > 0.2)
            b.fehlerbilder.append(QStringLiteral("Düse vermutlich verstopft (steigender Extruderstrom)"));
    }
    // Heizung ausgefallen: volle Heizleistung ohne Temperaturanstieg in der Aufheizphase.
    if (p.aufheizen.size() >= 30) {
        bool volleLeistung = true;
        double anstieg = 0;
        for (int i = 10; i < 30; ++i) {
            if (p.aufheizen[i].heizleistungProzent < 95) volleLeistung = false;
            anstieg = p.aufheizen[i].dueseIstC - p.aufheizen[10].dueseIstC;
        }
        if (volleLeistung && anstieg < 2.0)
            b.fehlerbilder.append(QStringLiteral("Heizung vermutlich ausgefallen (volle Leistung ohne Temperaturanstieg)"));
    }
    // Verrauschter Temperaturfühler: starke hochfrequente Schwankungen.
    if (p.druck.size() >= 20) {
        double rauschen = 0;
        for (int i = 2; i < p.druck.size(); ++i)
            rauschen += qAbs(p.druck[i].dueseIstC - p.druck[i - 1].dueseIstC);
        if (rauschen / (p.druck.size() - 2) > 1.5)
            b.fehlerbilder.append(QStringLiteral("Temperaturfühler verrauscht (starkes Messrauschen)"));
    }
    // Unpassende Betttemperatur zum Material.
    if (material && qAbs(p.bettSollC - material->bettTempC) > 10)
        b.fehlerbilder.append(QStringLiteral("Betttemperatur weicht stark von Materialempfehlung ab (%.0f vs. %.0f °C)")
                               .arg(p.bettSollC).arg(material->bettTempC));
    return b;
}

Kostenrechnung::Ergebnis Kostenrechnung::auftrag(const Auftrag &a, const Material *mat,
                                                  double stundensatzEur)
{
    Ergebnis e;
    if (mat)
        e.materialEur = a.filamentSollG / 1000.0 * mat->preisEurKg;
    double dauerMin = a.dauerSollMin;
    if (a.start.isValid() && a.ende.isValid())
        dauerMin = a.start.secsTo(a.ende) / 60.0;
    e.maschinenEur = dauerMin / 60.0 * stundensatzEur;
    e.gesamtEur = e.materialEur + e.maschinenEur;
    return e;
}

QList<Planung::Zuweisung> Planung::greedy(const QList<Auftrag> &ausstehend,
                                          const QList<Drucker> &drucker,
                                          const DataStore &store,
                                          const QDateTime &startZeit)
{
    QList<Zuweisung> out;
    // Belegung je Drucker (Start mit startZeit)
    QHash<QString, QDateTime> freiAb;
    for (const auto &d : drucker)
        freiAb.insert(d.druckerId, startZeit);
    // Aufträge nach Dauer sortieren (greedy: frühester Fertigstellungszeitpunkt)
    QList<Auftrag> liste = ausstehend;
    std::sort(liste.begin(), liste.end(), [](const Auftrag &a, const Auftrag &b) {
        return a.dauerSollMin < b.dauerSollMin;
    });
    for (const auto &a : liste) {
        const Material *mat = store.material(a.materialId);
        QString bestId;
        QDateTime bestFertig;
        for (const auto &d : drucker) {
            if (d.status != QLatin1String("bereit")) continue;
            const Modell *m = store.modell(d.modellId);
            if (!m || !mat) continue;
            if (mat->dueseTempC > m->maxDueseTempC || mat->bettTempC > m->maxBettTempC)
                continue;   // Material nicht für dieses Modell geeignet
            const QDateTime von = freiAb.value(d.druckerId);
            const QDateTime bis = von.addSecs(qint64(a.dauerSollMin * 60));
            if (bestId.isEmpty() || bis < bestFertig) {
                bestId = d.druckerId;
                bestFertig = bis;
            }
        }
        if (!bestId.isEmpty()) {
            out.append({a.auftragId, bestId, freiAb.value(bestId), bestFertig});
            freiAb.insert(bestId, bestFertig);
        }
    }
    return out;
}

PidSim::Ergebnis PidSim::simuliere(double kp, double ki, double kd,
                                   double tsoll, double t0,
                                   double tauS, double dtS, double dauerS)
{
    // Strecke 1. Ordnung: dT/dt = (u/Leistungsklasse - (T - T0))/tau, vereinfacht:
    // dT/dt = (u * kHeiz - (T - T0)) / tau mit kHeiz so, dass 100 % die Solltemp. hält.
    Ergebnis e;
    const double kHeiz = (tsoll - t0) / 100.0;   // 100 % Leistung hält Tsoll (stationär)
    double T = t0, integral = 0, ePrev = tsoll - t0, t = 0;
    const int n = int(dauerS / dtS) + 1;
    e.zeitS.reserve(n);
    e.istC.reserve(n);
    double tmax = t0;
    for (int i = 0; i < n; ++i) {
        const double err = tsoll - T;
        integral += err * dtS;
        const double deriv = (i == 0) ? 0.0 : (err - ePrev) / dtS;
        double u = kp * err + ki * integral + kd * deriv;
        u = qBound(0.0, u, 100.0);
        const double dT = (u * kHeiz - (T - t0)) / tauS * dtS;
        T += dT;
        ePrev = err;
        t = i * dtS;
        e.zeitS.append(t);
        e.istC.append(T);
        tmax = qMax(tmax, T);
    }
    e.ueberschwingenK = tmax - tsoll;
    e.anstiegszeitS = RegelgueteRechner::anstiegszeitS(e.zeitS, e.istC, t0, tsoll);
    return e;
}
