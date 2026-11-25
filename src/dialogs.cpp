// Implementierung siehe dialogs.h.

#include "dialogs.h"
#include "tablemodels.h"
#include "plotwidget.h"
#include "regelguete.h"
#include "analyse.h"
#include "pidsim.h"
#include "csvio.h"
#include <QFormLayout>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QValueAxis>
#include <QGroupBox>
#include <QSplitter>
#include <QListWidget>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Anmelden"));
    auto *form = new QFormLayout(this);
    m_role = new QComboBox(this);
    m_role->addItem(tr("Bediener"), Auth::Operator);
    m_role->addItem(tr("Technik"), Auth::Engineer);
    m_role->addItem(tr("Admin"), Auth::Admin);
    m_name = new QLineEdit(this);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Rolle:"), m_role);
    form->addRow(tr("Name:"), m_name);
    form->addRow(tr("Passwort:"), m_password);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(btns);
}
Auth::Role LoginDialog::selectedRole() const
{ return static_cast<Auth::Role>(m_role->currentData().toInt()); }
QString LoginDialog::name() const { return m_name->text().trimmed(); }
QString LoginDialog::password() const { return m_password->text(); }

PrinterDetailDialog::PrinterDetailDialog(const Printer &d, const DataStore *store,
                                         ProtocolLoader *loader, QWidget *parent)
    : QDialog(parent), m_store(store), m_loader(loader), m_printerId(d.printerId)
{
    setWindowTitle(tr("Drucker %1").arg(d.printerId));
    resize(900, 600);
    auto *layout = new QVBoxLayout(this);
    m_masterDataLabel = new QLabel(this);
    m_masterDataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_masterDataLabel);

    const DeviceModel *m = store->model(d.modelId);

    QString text = tr("<b>%1</b> - Modell: %2 (%3, %4) | Standort: %5 | Inbetriebnahme: %6 | "
                      "Betriebsstunden: %7 | Düse: %8 mm | Firmware: %9 | PID: Kp=%10 Ki=%11 Kd=%12 | Status: %13")
        .arg(d.printerId.toHtmlEscaped(), m ? m->description.toHtmlEscaped() : tr("unbekannt"),
             m ? m->manufacturer.toHtmlEscaped() : QString(), m ? m->kinematics.toHtmlEscaped() : QString(),
             d.location.toHtmlEscaped(), CsvIo::formatDate(d.commissionedOn))
        .arg(qRound64(d.operatingHours)).arg(qRound64(d.nozzleMm)).arg(d.firmware.toHtmlEscaped())
        .arg(qRound64(d.pidKp)).arg(qRound64(d.pidKi)).arg(qRound64(d.pidKd)).arg(d.status.toHtmlEscaped());
    if (m)
        text += tr("<br>Bauraum: %1×%2×%3 mm | max. Düse %4 °C | max. Bett %5 °C")
                    .arg(qRound64(m->buildXMm)).arg(qRound64(m->buildYMm)).arg(qRound64(m->buildZMm))
                    .arg(qRound64(m->maxNozzleTempC)).arg(qRound64(m->maxBedTempC));
    m_masterDataLabel->setText(text);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_model = new JobModel(store, this);
    m_model->setPrinterOnly(d.printerId);
    m_jobsView = new QTableView(this);
    m_jobsView->setAlternatingRowColors(true);
    m_jobsView->setModel(m_model);
    m_jobsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_jobsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_jobsView, &QTableView::clicked, this, [this](const QModelIndex &idx) {
        if (const Job *a = m_model->jobAtRow(idx.row())) showJob(*a);
    });
    splitter->addWidget(m_jobsView);

    auto *maintenanceBox = new QGroupBox(tr("Wartungen"), this);
    auto *wl = new QVBoxLayout(maintenanceBox);
    m_maintenanceModel = new MaintenanceModel(store, maintenanceBox);
    m_maintenanceModel->setPrinterOnly(d.printerId);
    m_maintenancesView = new QTableView(maintenanceBox);
    m_maintenancesView->setAlternatingRowColors(true);
    m_maintenancesView->setModel(m_maintenanceModel);
    m_maintenancesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_maintenancesView->setSelectionBehavior(QAbstractItemView::SelectRows);
    wl->addWidget(m_maintenancesView);
    splitter->addWidget(maintenanceBox);

    auto *unten = new QSplitter(Qt::Horizontal, this);
    m_plot = new PlotWidget(unten);
    auto *rechts = new QWidget(unten);
    auto *rl = new QVBoxLayout(rechts);
    m_qualityLabel = new QLabel(rechts);
    m_findingsLabel = new QLabel(rechts);
    m_findingsLabel->setWordWrap(true);
    rl->addWidget(m_qualityLabel);
    rl->addWidget(m_findingsLabel);
    unten->addWidget(m_plot);
    unten->addWidget(rechts);
    splitter->addWidget(unten);
    layout->addWidget(splitter);
    m_plot->clear();

    for (const auto &a : store->jobsOfPrinter(d.printerId)) {
        if (a.jobResult != QLatin1String("ausstehend") && !loader->isCached(a.jobId))
            loader->loadAsync(a.jobId);
    }
    connect(loader, &ProtocolLoader::fertig, this, [this](const QString &) {

    });
}

void PrinterDetailDialog::showJob(const Job &a)
{
    if (a.jobResult == QLatin1String("ausstehend")) {
        m_plot->clear();
        m_qualityLabel->setText(tr("Auftrag %1 ist geplant - noch kein Protokoll vorhanden.").arg(a.jobId));
        m_findingsLabel->clear();
        return;
    }
    const PrintLog p = m_loader->load(a.jobId);
    if (!p.valid) {
        m_plot->clear();
        m_qualityLabel->setText(tr("Protokoll nicht verfügbar: %1").arg(p.error.toHtmlEscaped()));
        m_findingsLabel->clear();
        return;
    }
    m_plot->showProtocol(p);
    const ControlQuality rg = ControlQualityCalculator::computeValues(p.heatUp, p.printPhase, p.nozzleTargetC);
    QString t;
    if (rg.available) {
        t = tr("Regelgüte %1:<br>Anstiegszeit: %2 s<br>Überschwingen: %3 K (%4 %)<br>"
               "Ausregelzeit: %5<br Mittlere Abweichung Druck: %6 K<br>Maximale Abweichung: %7 K")
            .arg(a.jobId.toHtmlEscaped()).arg(qRound64(rg.riseTimeS)).arg(qRound64(rg.overshootK))
            .arg(qRound64(rg.overshootPercent))
            .arg(rg.settled ? QString::number(rg.settlingTimeS, 'f', 1) + QStringLiteral(" s")
                                : tr("nicht ausgeregelt"))
            .arg(qRound64(rg.meanDeviationK)).arg(qRound64(rg.maxDeviationK));
        if (!rg.note.isEmpty())
            t += QStringLiteral("<br>%1").arg(rg.note);
    } else {
        t = tr("Regelgüte nicht berechenbar für %1").arg(a.jobId.toHtmlEscaped());
    }
    m_qualityLabel->setText(t);
    const auto bef = ErrorDetection::detectFaults(p, rg, m_store->material(a.materialId));
    QStringList fb;
    if (bef.poorlyTuned)
        fb.append(tr("Regler schlecht eingestellt (Überschwingen/Ausregelzeit prüfen)"));
    fb.append(bef.errorPatterns);
    m_findingsLabel->setText(fb.isEmpty() ? tr("Keine Fehlerbilder erkannt.")
                                     : fb.join(QStringLiteral("<br>")));
}

PrinterEditDialog::PrinterEditDialog(const Printer &d, QWidget *parent) : QDialog(parent)
    , m_printerId(d.printerId)
{
    setWindowTitle(tr("Stammdaten bearbeiten: %1").arg(d.printerId));
    auto *form = new QFormLayout(this);
    m_locationEdit = new QLineEdit(d.location, this);
    m_status = new QComboBox(this);
    m_status->addItems({QStringLiteral("bereit"), QStringLiteral("druckt"),
                        QStringLiteral("wartung"), QStringLiteral("defekt")});
    if (m_status->findText(d.status) < 0)
        m_status->addItem(d.status);
    m_status->setCurrentText(d.status);
    m_kp = new QLineEdit(QString::number(d.pidKp), this);
    m_ki = new QLineEdit(QString::number(d.pidKi), this);
    m_kd = new QLineEdit(QString::number(d.pidKd), this);
    form->addRow(tr("Standort:"), m_locationEdit);
    form->addRow(tr("Status:"), m_status);
    form->addRow(tr("PID Kp:"), m_kp);
    form->addRow(tr("PID Ki:"), m_ki);
    form->addRow(tr("PID Kd:"), m_kd);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(btns);
}

Printer PrinterEditDialog::editedPrinter() const
{
    Printer d;
    d.printerId = m_printerId;
    d.location = m_locationEdit->text().trimmed();
    d.status = m_status->currentText();
    bool okKp = false, okKi = false, okKd = false;
    d.pidKp = m_kp->text().toDouble(&okKp);
    d.pidKi = m_ki->text().toDouble(&okKi);
    d.pidKd = m_kd->text().toDouble(&okKd);
    const_cast<PrinterEditDialog*>(this)->m_gueltig = okKp && okKi && okKd;
    return d;
}

PidSimDialog::PidSimDialog(const Printer &d, const DataStore *store,
                           ProtocolLoader *loader, QWidget *parent)
    : QDialog(parent), m_modelId(d.modelId), m_printerId(d.printerId),
      m_store(store), m_loader(loader)
{
    setWindowTitle(tr("PID-Simulation: %1").arg(d.printerId));
    resize(900, 650);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();
    m_kp = new QLineEdit(QString::number(d.pidKp), this);
    m_ki = new QLineEdit(QString::number(d.pidKi), this);
    m_kd = new QLineEdit(QString::number(d.pidKd), this);
    m_targetTemp = new QLineEdit(QStringLiteral("210"), this);
    form->addRow(tr("Kp:"), m_kp);
    form->addRow(tr("Ki:"), m_ki);
    form->addRow(tr("Kd:"), m_kd);
    form->addRow(tr("Solltemperatur [°C]:"), m_targetTemp);
    m_disturbanceCheck = new QCheckBox(tr("Störgröße: Extrusion (Druckphase nach Aufheizen)"), this);
    m_noiseCheck = new QCheckBox(tr("Störgröße: Fühlerrauschen ±0,15 K"), this);
    m_fanCheck = new QCheckBox(tr("Störgröße: Lüfter 100 % (Konvektion)"), this);
    m_fit = new QCheckBox(tr("Strecke aus letztem echten Aufheizprotokoll dieses Druckers fitten"), this);
    form->addRow(m_disturbanceCheck);
    form->addRow(m_noiseCheck);
    form->addRow(m_fanCheck);
    form->addRow(m_fit);
    connect(m_disturbanceCheck, &QCheckBox::toggled, this, &PidSimDialog::startSimulation);
    connect(m_noiseCheck, &QCheckBox::toggled, this, &PidSimDialog::startSimulation);
    connect(m_fanCheck, &QCheckBox::toggled, this, &PidSimDialog::startSimulation);
    connect(m_fit, &QCheckBox::toggled, this, &PidSimDialog::startSimulation);
    auto *btn = new QPushButton(tr("Simulieren"), this);
    connect(btn, &QPushButton::clicked, this, &PidSimDialog::startSimulation);
    form->addRow(btn);
    layout->addLayout(form);

    auto *unten = new QSplitter(Qt::Horizontal, this);
    m_plot = new PlotWidget(unten);
    auto *rechts = new QWidget(unten);
    auto *rl = new QVBoxLayout(rechts);
    auto *rankTitle = new QLabel(tr("Rangliste (beste zuerst):"), rechts);
    m_rankingList = new QListWidget(rechts);
    rl->addWidget(rankTitle);
    rl->addWidget(m_rankingList);
    unten->addWidget(m_plot);
    unten->addWidget(rechts);
    layout->addWidget(unten, 1);
    m_resultLabel = new QLabel(this);
    m_resultLabel->setWordWrap(true);
    layout->addWidget(m_resultLabel);
    startSimulation();
}

// Strecke: Modell-Preset, optional Fit aus dem neuesten echten Aufheizprotokoll
// dieses Druckers (Fallback kommentiert im Label).
PidSim::Options PidSimDialog::resolvePlantOptions(QString &info) const
{
    PidSim::Options opt = PidSim::plantPresetForModel(m_modelId);
    info = tr("Strecke: Modell-Preset (kein Protokoll gefunden).");
    if (!m_fit->isChecked() || !m_store || !m_loader)
        return opt;

    const Job *latest = nullptr;
    for (const auto &a : m_store->jobsOfPrinter(m_printerId)) {
        if (a.jobResult == QLatin1String("ausstehend") || !a.ende.isValid()) continue;
        if (m_loader->pathForJob(a.jobId).isEmpty()) continue;
        if (!latest || a.ende > latest->ende) latest = &a;
    }
    if (!latest)
        return opt;
    const PrintLog p = m_loader->load(latest->jobId);
    if (!p.valid)
        return opt;
    const auto fit = PidSim::fitPlantFromHeatUp(p.heatUp, p.nozzleTargetC, opt);
    info = tr("Strecke aus Protokoll %1 gefitten (SSE %2).")
               .arg(latest->jobId).arg(QString::number(fit.sse, 'f', 1));
    return fit.opt;
}

static QVector<Sample> curveFromResult(const PidSim::Result &res, bool output)
{
    QVector<Sample> punkte;
    punkte.reserve(res.timeS.size());
    for (int j = 0; j < res.timeS.size(); ++j)
        punkte.append({res.timeS[j], output ? res.outputPercent[j] : res.actualC[j],
                       0, 0, 0, 0, 0, 0});
    return punkte;
}

void PidSimDialog::displayRanking(const QList<PidSim::RankEntry> &ranking, const QString &plantInfo)
{
    QList<QPair<QString, QVector<Sample>>> curves;
    for (int i = 0; i < ranking.size(); ++i)
        curves.append({tr("Rang %1: Kp=%2 Ki=%3 Kd=%4").arg(i + 1).arg(ranking[i].kp).arg(ranking[i].ki).arg(ranking[i].kd),
                       curveFromResult(ranking[i].res, false)});
    if (!ranking.isEmpty())
        curves.append({tr("Stellgröße u [%] (Rang 1)"), curveFromResult(ranking.first().res, true)});
    m_plot->showComparison(curves);
    if (auto *ay = qobject_cast<QValueAxis*>(m_plot->chart()->axes(Qt::Vertical).first()))
        ay->setTitleText(tr("Temperatur [°C] / u [%]"));

    auto settlingText = [this](const PidSim::Result &e) {
        return e.settlingTimeS >= 0
                ? QString::number(e.settlingTimeS, 'f', 1) + QStringLiteral(" s")
                : tr("nicht ausgeregelt");
    };
    m_rankingList->clear();
    for (int i = 0; i < ranking.size(); ++i)
        m_rankingList->addItem(tr("%1. Kp=%2 Ki=%3 Kd=%4 – Ausregelzeit %5, Überschwingen %6 K")
                                 .arg(i + 1).arg(ranking[i].kp).arg(ranking[i].ki).arg(ranking[i].kd)
                                 .arg(settlingText(ranking[i].res))
                                 .arg(QString::number(ranking[i].res.overshootK, 'f', 1)));
    if (!ranking.isEmpty()) {
        const auto &best = ranking.first();
        m_resultLabel->setText(tr("Simuliert: Anstiegszeit %1 s, Überschwingen %2 K, Ausregelzeit %3 s")
                                .arg(QString::number(best.res.riseTimeS, 'f', 1))
                                .arg(QString::number(best.res.overshootK, 'f', 1))
                                .arg(settlingText(best.res))
                            + QStringLiteral("<br>%1").arg(plantInfo));
    }
}

void PidSimDialog::startSimulation()
{
    const double targetC = m_targetTemp->text().toDouble();
    const double kp = m_kp->text().toDouble();
    const double ki = m_ki->text().toDouble();
    const double kd = m_kd->text().toDouble();
    QString plantInfo;
    PidSim::Options opt = resolvePlantOptions(plantInfo);
    opt.printPhaseDisturbance = m_disturbanceCheck->isChecked();
    opt.sensorNoise = m_noiseCheck->isChecked();
    opt.fanPercent = m_fanCheck->isChecked() ? 100.0 : 0.0;

    const auto ranking = PidSim::rankParameterCandidates(PidSim::varyParametersAround(kp, ki, kd), targetC, 24.0, opt);
    displayRanking(ranking, plantInfo);
}

ReportDialog::ReportDialog(const DataStore *store, QWidget *parent)
    : QDialog(parent), m_store(store)
{
    setWindowTitle(tr("Bericht exportieren"));
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();
    m_type = new QComboBox(this);
    m_type->addItem(tr("Wochenbericht Druckerfarm (CSV)"), 0);
    m_type->addItem(tr("Kostenübersicht je Auftrag (CSV)"), 1);
    m_type->addItem(tr("Wartungsplan (CSV)"), 2);
    form->addRow(tr("Bericht:"), m_type);
    auto *btn = new QPushButton(tr("Exportieren..."), this);
    connect(btn, &QPushButton::clicked, this, &ReportDialog::exportReport);
    form->addRow(btn);
    layout->addLayout(form);
}

void ReportDialog::exportReport()
{

    if (!Auth::mayExport()) {
        m_store->log(LogEntry::System, tr("Export verweigert (nicht angemeldet): Bericht-Export"));
        QMessageBox::information(this, tr("Rechte"), tr("Bitte zuerst anmelden."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Bericht speichern"),
                                                      QStringLiteral("bericht.csv"), tr("CSV (*.csv)"));
    if (path.isEmpty()) return;
    QList<QStringList> rows;
    const int type = m_type->currentData().toInt();
    if (type == 0) {
        rows.append({tr("drucker_id"), tr("erfolgsquote"), tr("filament_g"), tr("materialkosten_eur")});
        for (const auto &d : m_store->printer()) {
            const double q = m_store->successRateOfPrinter(d.printerId);
            rows.append({d.printerId, q < 0 ? QStringLiteral("-") : QString::number(q, 'f', 3),
                         QString::number(m_store->filamentUsedGrams(d.printerId), 'f', 1),
                         QString::number(m_store->materialCostEur(d.printerId), 'f', 2)});
        }
    } else if (type == 1) {
        rows.append({tr("auftrag_id"), tr("drucker_id"), tr("material_eur"), tr("maschinen_eur"), tr("gesamt_eur")});
        for (const auto &a : m_store->auftraege()) {
            const auto k = CostCalculator::computeJobCost(a, m_store->material(a.materialId));
            rows.append({a.jobId, a.printerId, QString::number(k.materialEur, 'f', 2),
                         QString::number(k.maschinenEur, 'f', 2), QString::number(k.gesamtEur, 'f', 2)});
        }
    } else {
        rows.append({tr("wartung_id"), tr("drucker_id"), tr("datum"), tr("art"), tr("dauer_min"), tr("bemerkung")});
        for (const auto &w : m_store->maintenances())
            rows.append({w.maintenanceId, w.printerId, CsvIo::formatDate(w.date), w.kind,
                         QString::number(w.durationMin), w.note});
    }
    QString err;
    if (CsvIo::writeCsv(path, rows, &err))
        QMessageBox::information(this, tr("Export"), tr("Bericht geschrieben: %1").arg(path));
    else
        QMessageBox::warning(this, tr("Export"), err);
}
