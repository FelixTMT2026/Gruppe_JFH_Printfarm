// Implementierung siehe teilewidget.h.

#include "teilewidget.h"
#include "plotwidget.h"
#include "ganttwidget.h"
#include "regelguete.h"
#include "analyse.h"
#include "csvio.h"
#include "auth.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>

PartsWidget::PartsWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent)
    : QWidget(parent), m_store(store), m_loader(loader), m_currentJob(QString())
{
    auto *layout = new QVBoxLayout(this);

    auto *filter = new QHBoxLayout();
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Suche nach Auftrag, Drucker, Bauteil..."));
    m_search->setClearButtonEnabled(true);
    m_resultFilter = new QComboBox(this);
    m_resultFilter->addItem(tr("Alle Ergebnisse"), QString());
    m_resultFilter->addItem(QStringLiteral("erfolgreich"), QStringLiteral("erfolgreich"));
    m_resultFilter->addItem(QStringLiteral("abgebrochen"), QStringLiteral("abgebrochen"));
    m_resultFilter->addItem(QStringLiteral("fehlgeschlagen"), QStringLiteral("fehlgeschlagen"));
    m_resultFilter->addItem(QStringLiteral("ausstehend"), QStringLiteral("ausstehend"));
    m_planBtn = new QPushButton(tr("Belegungsplan (Gantt)"), this);
    m_planBtn->setToolTip(tr("Geplante Aufträge greedy auf geeignete Drucker verteilen (nur Admin)"));

    m_planBtn->setVisible(Auth::role() == Auth::Admin);
    m_resetBtn = new QPushButton(tr("Filter zurücksetzen"), this);
    m_resetBtn->setToolTip(tr("Suche und Ergebnis-Filter auf „Alle“ zurücksetzen"));
    filter->addWidget(m_search, 1);
    filter->addWidget(m_resultFilter);
    filter->addWidget(m_resetBtn);
    filter->addWidget(m_planBtn);
    layout->addLayout(filter);

    auto *detailBox = new QGroupBox(tr("Protokoll / Verlauf / Auswertung"), this);
    auto *dl = new QVBoxLayout(detailBox);
    m_info = new QLabel(detailBox);
    m_info->setWordWrap(true);
    m_plot = new PlotWidget(detailBox);
    m_costLabel = new QLabel(detailBox);
    m_costLabel->setWordWrap(true);
    dl->addWidget(m_info);
    dl->addWidget(m_plot, 1);
    dl->addWidget(m_costLabel);

    m_model = new JobModel(store, this);
    connect(store, &DataStore::dataLoaded, m_model, &JobModel::refresh);
    connect(store, &DataStore::dataLoaded, this, &PartsWidget::initialAuswahl);
    m_view = new QTableView(this);
    m_view->setAlternatingRowColors(true);
    m_view->setModel(m_model);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(detailBox);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setChildrenCollapsible(false);
    layout->addWidget(splitter);
    m_view->setSortingEnabled(true);

    connect(m_search, &QLineEdit::textChanged, this, [this]() {
        m_model->setFilter(m_search->text(), m_resultFilter->currentData().toString());
    });
    connect(m_resultFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setFilter(m_search->text(), m_resultFilter->currentData().toString());
    });
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        m_search->clear();
        m_resultFilter->setCurrentIndex(0);
    });

    connect(m_view, &QTableView::clicked, this, [this](const QModelIndex &idx) {
        if (const Job *a = m_model->jobAtRow(idx.row())) showJob(*a);
    });
    connect(m_planBtn, &QPushButton::clicked, this, &PartsWidget::showSchedule);
    connect(m_loader, &ProtocolLoader::fertig, this, [this](const QString &id) {
        emit statusMessage(tr("Protokoll %1 im Hintergrund geladen (Cache).").arg(id));

        if (id == m_currentJob) {
            auto *tabs = findChild<QTabWidget*>();
            Q_UNUSED(tabs);
            for (const auto &a : m_store->auftraege())
                if (a.jobId == id) { showJob(a); break; }
        }
    });
    connect(m_loader, &ProtocolLoader::error, this, [this](const QString &id, const QString &f) {
        m_store->log(LogEntry::PrintLog, f, id);
        emit statusMessage(tr("Protokollfehler %1: %2").arg(id, f));

        if (id == m_currentJob) {
            m_plot->clear();
            m_info->setText(m_info->text() + tr("<br><i>Kein Protokoll verfügbar: %1</i>").arg(f.toHtmlEscaped()));
        }
    });
    m_plot->clear();

    if (!store->auftraege().isEmpty())
        initialAuswahl();
}

void PartsWidget::showJob(const Job &a)
{
    m_currentJob = a.jobId;
    const Material *mat = m_store->material(a.materialId);

    m_info->setText(tr("<b>%1</b> - %2 auf %3 | Material: %4 | Soll: %5 min, %6 g | Ergebnis: %7%8")
        .arg(a.jobId.toHtmlEscaped(), a.part.toHtmlEscaped(), a.printerId.toHtmlEscaped(),
             (mat ? mat->description : a.materialId).toHtmlEscaped())
        .arg(qRound64(a.plannedMinutes)).arg(qRound64(a.plannedFilamentG)).arg(a.jobResult.toHtmlEscaped())
        .arg(a.errorCode.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(a.errorCode.toHtmlEscaped())));

    const auto k = CostCalculator::computeJobCost(a, mat);
    m_costLabel->setText(tr("Kosten: Material %1 € + Maschine %2 € = %3 €")
        .arg(qRound64(k.materialEur)).arg(qRound64(k.maschinenEur)).arg(qRound64(k.gesamtEur)));

    if (a.jobResult == QLatin1String("ausstehend")) {
        m_plot->clear();
        return;
    }
    if (!m_loader->isCached(a.jobId)) {

        m_loader->loadAsync(a.jobId);
        m_plot->clear();
        m_info->setText(m_info->text() + tr("<br><i>Protokoll wird im Hintergrund geladen...</i>"));
        return;
    }
    const PrintLog p = m_loader->load(a.jobId);
    if (!p.valid) {
        m_plot->clear();
        m_info->setText(m_info->text() + tr("<br><i>Kein Protokoll verfügbar: %1</i>").arg(p.error.toHtmlEscaped()));
        return;
    }
    m_plot->showProtocol(p);
    const ControlQuality rg = ControlQualityCalculator::computeValues(p.heatUp, p.printPhase, p.nozzleTargetC);
    if (rg.available) {
        m_costLabel->setText(m_costLabel->text() + tr("<br>Regelgüte: tr=%1 s, Überschwingen=%2 K (%3 %), Ausregelzeit=%4, Ø-Abweichung=%5 K, max=%6 K")
            .arg(qRound64(rg.riseTimeS)).arg(qRound64(rg.overshootK))
            .arg(qRound64(rg.overshootPercent))
            .arg(rg.settled ? QString::number(rg.settlingTimeS, 'f', 0) + " s" : tr("nicht ausgeregelt"))
            .arg(qRound64(rg.meanDeviationK)).arg(qRound64(rg.maxDeviationK)));
        const auto bef = ErrorDetection::detectFaults(p, rg, mat);
        if (bef.poorlyTuned || !bef.errorPatterns.isEmpty()) {
            QStringList fb;
            if (bef.poorlyTuned) fb.append(tr("Regler schlecht eingestellt"));
            fb.append(bef.errorPatterns);
            m_store->log(LogEntry::PrintLog,
                tr("Auftrag %1: %2").arg(a.jobId, fb.join(QStringLiteral("; "))), a.jobId);
        }
    }
}

void PartsWidget::initialAuswahl()
{

    m_model->sort(JobModel::ColStart, Qt::DescendingOrder);

    const Job *last = nullptr;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        const Job *a = m_model->jobAtRow(i);
        if (!a || a->jobResult == QLatin1String("ausstehend") || !a->ende.isValid())
            continue;
        if (!last || a->ende > last->ende)
            last = a;
    }
    if (!last)
        return;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (m_model->jobAtRow(i) == last) {
            m_view->setCurrentIndex(m_model->index(i, 0));
            m_view->selectRow(i);
            showJob(*last);
            break;
        }
    }
}

void PartsWidget::showSchedule()
{

    if (Auth::role() != Auth::Admin) {
        QMessageBox::information(this, tr("Rechte"),
            tr("Der Belegungsplan ist nur für die Rolle „admin“ verfügbar."));
        return;
    }
    QList<Job> pending;
    for (const auto &a : m_store->auftraege())
        if (a.jobResult == QLatin1String("ausstehend"))
            pending.append(a);
    const auto assignments = Planning::assignJobsGreedy(pending, m_store->printer(), *m_store,
                                             QDateTime::currentDateTime());
    QStringList printerIds;
    for (const auto &z : assignments)
        if (!printerIds.contains(z.printerId)) printerIds.append(z.printerId);
    m_store->log(LogEntry::System,
        tr("Greedy-Planung: %1 geplante Aufträge auf %2 Zuweisungen verteilt")
            .arg(pending.size()).arg(assignments.size()));
    emit statusMessage(tr("Planung: %1 Zuweisungen (siehe Belegungsplan).").arg(assignments.size()));

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Belegungsplan (Gantt) - geplante Aufträge"));
    dlg.resize(900, 500);
    auto *dl = new QVBoxLayout(&dlg);
    auto *gantt = new GanttWidget(&dlg);
    gantt->setSchedule(assignments, printerIds);
    dl->addWidget(gantt);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    dl->addWidget(btns);
    dlg.exec();
}
