#include "dialogs.h"
#include "tablemodels.h"
#include "plotwidget.h"
#include "regelguete.h"
#include "analyse.h"
#include "csvio.h"
#include <QFormLayout>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>

// ---------------------------------------------------------------- LoginDialog
LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Anmelden"));
    auto *form = new QFormLayout(this);
    m_rolle = new QComboBox(this);
    m_rolle->addItem(tr("Bediener"), Auth::Bediener);
    m_rolle->addItem(tr("Technik"), Auth::Technik);
    m_rolle->addItem(tr("Admin"), Auth::Admin);
    m_name = new QLineEdit(this);
    form->addRow(tr("Rolle:"), m_rolle);
    form->addRow(tr("Name:"), m_name);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(btns);
}
Auth::Rolle LoginDialog::ausgewaehlteRolle() const
{ return static_cast<Auth::Rolle>(m_rolle->currentData().toInt()); }
QString LoginDialog::name() const { return m_name->text().trimmed(); }

// ---------------------------------------------------------------- DruckerDetailDialog
DruckerDetailDialog::DruckerDetailDialog(const Drucker &d, const DataStore *store,
                                         ProtocolLoader *loader, QWidget *parent)
    : QDialog(parent), m_store(store), m_loader(loader), m_druckerId(d.druckerId)
{
    setWindowTitle(tr("Drucker %1").arg(d.druckerId));
    resize(900, 600);
    auto *layout = new QVBoxLayout(this);
    m_stammdaten = new QLabel(this);
    m_stammdaten->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_stammdaten);

    const Modell *m = store->modell(d.modellId);
    QString text = tr("<b>%1</b> - Modell: %2 (%3, %4) | Standort: %5 | Inbetriebnahme: %6 | "
                      "Betriebsstunden: %7 | Düse: %8 mm | Firmware: %9 | PID: Kp=%10 Ki=%11 Kd=%12 | Status: %13")
        .arg(d.druckerId, m ? m->bezeichnung : tr("unbekannt"), m ? m->hersteller : QString(),
             m ? m->kinematik : QString(), d.standort, CsvIo::formatDate(d.inbetriebnahme))
        .arg(qRound64(d.betriebsstunden)).arg(qRound64(d.dueseMm)).arg(d.firmware)
        .arg(qRound64(d.pidKp)).arg(qRound64(d.pidKi)).arg(qRound64(d.pidKd)).arg(d.status);
    if (m)
        text += tr("<br>Bauraum: %1×%2×%3 mm | max. Düse %4 °C | max. Bett %5 °C")
                    .arg(qRound64(m->bauraumXMm)).arg(qRound64(m->bauraumYMm)).arg(qRound64(m->bauraumZMm))
                    .arg(qRound64(m->maxDueseTempC)).arg(qRound64(m->maxBettTempC));
    m_stammdaten->setText(text);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_model = new AuftragModel(store, this);
    m_model->setNurDrucker(d.druckerId);
    m_auftraegeView = new QTableView(this);
    m_auftraegeView->setModel(m_model);
    m_auftraegeView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_auftraegeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_auftraegeView, &QTableView::doubleClicked, this, [this](const QModelIndex &idx) {
        if (const Auftrag *a = m_model->auftragAt(idx.row())) zeigeAuftrag(*a);
    });
    splitter->addWidget(m_auftraegeView);

    auto *unten = new QSplitter(Qt::Horizontal, this);
    m_plot = new PlotWidget(unten);
    auto *rechts = new QWidget(unten);
    auto *rl = new QVBoxLayout(rechts);
    m_regelguete = new QLabel(rechts);
    m_befunde = new QLabel(rechts);
    m_befunde->setWordWrap(true);
    rl->addWidget(m_regelguete);
    rl->addWidget(m_befunde);
    unten->addWidget(m_plot);
    unten->addWidget(rechts);
    splitter->addWidget(unten);
    layout->addWidget(splitter);
    m_plot->leeren();

    // Protokolle im Hintergrund vorladen (LRU-Cache)
    for (const auto &a : store->auftraegeOfDrucker(d.druckerId)) {
        if (a.ergebnis != QLatin1String("ausstehend") && !loader->cached(a.auftragId))
            loader->loadAsync(a.auftragId);
    }
    connect(loader, &ProtocolLoader::fertig, this, [this](const QString &) {
        // Cache aktualisiert; Anzeige aktualisiert sich beim nächsten Doppelklick
    });
}

void DruckerDetailDialog::zeigeAuftrag(const Auftrag &a)
{
    if (a.ergebnis == QLatin1String("ausstehend")) {
        m_plot->leeren();
        m_regelguete->setText(tr("Auftrag %1 ist geplant - noch kein Protokoll vorhanden.").arg(a.auftragId));
        m_befunde->clear();
        return;
    }
    const Protokoll p = m_loader->load(a.auftragId);
    if (!p.valid) {
        m_plot->leeren();
        m_regelguete->setText(tr("Protokoll nicht verfügbar: %1").arg(p.fehler));
        m_befunde->clear();
        return;
    }
    m_plot->zeigeProtokoll(p);
    const Regelguete rg = RegelgueteRechner::werteAus(p.aufheizen, p.druck, p.dueseSollC);
    QString t;
    if (rg.verfuegbar) {
        t = tr("Regelgüte %1:<br>Anstiegszeit: %2 s<br>Überschwingen: %3 K (%4 %)<br>"
               "Ausregelzeit: %5<br Mittlere Abweichung Druck: %6 K<br>Maximale Abweichung: %7 K")
            .arg(a.auftragId).arg(qRound64(rg.anstiegszeitS)).arg(qRound64(rg.ueberschwingenK))
            .arg(qRound64(rg.ueberschwingenProzent))
            .arg(rg.ausgeregelt ? QString::number(rg.ausregelzeitS, 'f', 1) + QStringLiteral(" s")
                                : tr("nicht ausgeregelt"))
            .arg(qRound64(rg.mittlereAbweichungK)).arg(qRound64(rg.maximaleAbweichungK));
        if (!rg.bemerkung.isEmpty())
            t += QStringLiteral("<br>%1").arg(rg.bemerkung);
    } else {
        t = tr("Regelgüte nicht berechenbar für %1").arg(a.auftragId);
    }
    m_regelguete->setText(t);
    const auto bef = Fehlererkennung::analysiere(p, rg, m_store->material(a.materialId));
    QStringList fb;
    if (bef.schlechtEingestellt)
        fb.append(tr("Regler schlecht eingestellt (Überschwingen/Ausregelzeit prüfen)"));
    fb.append(bef.fehlerbilder);
    m_befunde->setText(fb.isEmpty() ? tr("Keine Fehlerbilder erkannt.")
                                     : fb.join(QStringLiteral("<br>")));
}

// ---------------------------------------------------------------- DruckerEditDialog
DruckerEditDialog::DruckerEditDialog(const Drucker &d, QWidget *parent) : QDialog(parent)
    , m_druckerId(d.druckerId)
{
    setWindowTitle(tr("Stammdaten bearbeiten: %1").arg(d.druckerId));
    auto *form = new QFormLayout(this);
    m_standort = new QLineEdit(d.standort, this);
    m_status = new QComboBox(this);
    m_status->addItems({QStringLiteral("bereit"), QStringLiteral("druckt"),
                        QStringLiteral("wartung"), QStringLiteral("defekt")});
    m_status->setCurrentText(d.status);
    m_kp = new QLineEdit(QString::number(d.pidKp), this);
    m_ki = new QLineEdit(QString::number(d.pidKi), this);
    m_kd = new QLineEdit(QString::number(d.pidKd), this);
    form->addRow(tr("Standort:"), m_standort);
    form->addRow(tr("Status:"), m_status);
    form->addRow(tr("PID Kp:"), m_kp);
    form->addRow(tr("PID Ki:"), m_ki);
    form->addRow(tr("PID Kd:"), m_kd);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(btns);
}

Drucker DruckerEditDialog::ergebnis() const
{
    Drucker d;
    d.druckerId = m_druckerId;
    d.standort = m_standort->text().trimmed();
    d.status = m_status->currentText();
    d.pidKp = m_kp->text().toDouble();
    d.pidKi = m_ki->text().toDouble();
    d.pidKd = m_kd->text().toDouble();
    return d;
}

// ---------------------------------------------------------------- PidSimDialog
PidSimDialog::PidSimDialog(const Drucker &d, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("PID-Simulation: %1").arg(d.druckerId));
    resize(800, 500);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();
    m_kp = new QLineEdit(QString::number(d.pidKp), this);
    m_ki = new QLineEdit(QString::number(d.pidKi), this);
    m_kd = new QLineEdit(QString::number(d.pidKd), this);
    m_tsoll = new QLineEdit(QStringLiteral("210"), this);
    form->addRow(tr("Kp:"), m_kp);
    form->addRow(tr("Ki:"), m_ki);
    form->addRow(tr("Kd:"), m_kd);
    form->addRow(tr("Solltemperatur [°C]:"), m_tsoll);
    auto *btn = new QPushButton(tr("Simulieren"), this);
    connect(btn, &QPushButton::clicked, this, &PidSimDialog::simuliere);
    form->addRow(btn);
    layout->addLayout(form);
    m_plot = new PlotWidget(this);
    layout->addWidget(m_plot);
    m_ergebnis = new QLabel(this);
    layout->addWidget(m_ergebnis);
    simuliere();
}

void PidSimDialog::simuliere()
{
    const double tsoll = m_tsoll->text().toDouble();
    const auto erg = PidSim::simuliere(m_kp->text().toDouble(), m_ki->text().toDouble(),
                                       m_kd->text().toDouble(), tsoll, 24.0);
    QVector<Messpunkt> punkte;
    for (int i = 0; i < erg.zeitS.size(); ++i)
        punkte.append({erg.zeitS[i], erg.istC[i], 0, 0, 0, 0, 0, 0});
    m_plot->zeigeVergleich({{tr("Simulierte Sprungantwort"), punkte}});
    m_ergebnis->setText(tr("Simuliert: Anstiegszeit %1 s, Überschwingen %2 K")
                            .arg(qRound64(erg.anstiegszeitS)).arg(qRound64(erg.ueberschwingenK)));
}

// ---------------------------------------------------------------- BerichtDialog
BerichtDialog::BerichtDialog(const DataStore *store, QWidget *parent)
    : QDialog(parent), m_store(store)
{
    setWindowTitle(tr("Bericht exportieren"));
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();
    m_typ = new QComboBox(this);
    m_typ->addItem(tr("Wochenbericht Druckerfarm (CSV)"), 0);
    m_typ->addItem(tr("Kostenübersicht je Auftrag (CSV)"), 1);
    m_typ->addItem(tr("Wartungsplan (CSV)"), 2);
    form->addRow(tr("Bericht:"), m_typ);
    auto *btn = new QPushButton(tr("Exportieren..."), this);
    connect(btn, &QPushButton::clicked, this, &BerichtDialog::exportiere);
    form->addRow(btn);
    layout->addLayout(form);
}

void BerichtDialog::exportiere()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Bericht speichern"),
                                                      QStringLiteral("bericht.csv"), tr("CSV (*.csv)"));
    if (path.isEmpty()) return;
    QList<QStringList> rows;
    const int typ = m_typ->currentData().toInt();
    if (typ == 0) {
        rows.append({tr("drucker_id"), tr("erfolgsquote"), tr("filament_g"), tr("materialkosten_eur")});
        for (const auto &d : m_store->drucker()) {
            const double q = m_store->erfolgsquoteDrucker(d.druckerId);
            rows.append({d.druckerId, q < 0 ? QStringLiteral("-") : QString::number(q, 'f', 3),
                         QString::number(m_store->filamentverbrauchG(d.druckerId), 'f', 1),
                         QString::number(m_store->materialkostenEur(d.druckerId), 'f', 2)});
        }
    } else if (typ == 1) {
        rows.append({tr("auftrag_id"), tr("drucker_id"), tr("material_eur"), tr("maschinen_eur"), tr("gesamt_eur")});
        for (const auto &a : m_store->auftraege()) {
            const auto k = Kostenrechnung::auftrag(a, m_store->material(a.materialId));
            rows.append({a.auftragId, a.druckerId, QString::number(k.materialEur, 'f', 2),
                         QString::number(k.maschinenEur, 'f', 2), QString::number(k.gesamtEur, 'f', 2)});
        }
    } else {
        rows.append({tr("wartung_id"), tr("drucker_id"), tr("datum"), tr("art"), tr("dauer_min"), tr("bemerkung")});
        for (const auto &w : m_store->wartungen())
            rows.append({w.wartungId, w.druckerId, CsvIo::formatDate(w.datum), w.art,
                         QString::number(w.dauerMin), w.bemerkung});
    }
    QString err;
    if (CsvIo::write(path, rows, &err))
        QMessageBox::information(this, tr("Export"), tr("Bericht geschrieben: %1").arg(path));
    else
        QMessageBox::warning(this, tr("Export"), err);
}
