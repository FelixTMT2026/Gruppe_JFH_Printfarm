// Implementierung siehe druckerwidget.h.

#include "druckerwidget.h"
#include "dialogs.h"
#include "auth.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>

PrinterWidget::PrinterWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent)
    : QWidget(parent), m_store(store), m_loader(loader)
{
    auto *layout = new QVBoxLayout(this);

    auto *filter = new QHBoxLayout();
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Suche nach ID, Modell, Standort..."));
    m_search->setClearButtonEnabled(true);
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(tr("Alle Status"), QString());

    for (const auto &s : {QStringLiteral("bereit"), QStringLiteral("druckt"),
                          QStringLiteral("wartung"), QStringLiteral("defekt")})
        m_statusFilter->addItem(s, s);

    connect(store, &DataStore::dataLoaded, this, [this, store]() {
        for (const auto &d : store->printer()) {
            if (d.status.isEmpty()) continue;
            bool bekannt = false;
            for (int i = 0; i < m_statusFilter->count(); ++i)
                if (m_statusFilter->itemData(i).toString() == d.status) bekannt = true;
            if (!bekannt)
                m_statusFilter->addItem(QStringLiteral("Status: %1").arg(d.status), d.status);
        }
    });
    m_kinematicsFilter = new QComboBox(this);
    m_kinematicsFilter->addItem(tr("Alle Kinematiken"), QString());
    for (const auto &k : {QStringLiteral("kartesisch"), QStringLiteral("corexy"),
                          QStringLiteral("delta")})
        m_kinematicsFilter->addItem(k, k);
    m_resetBtn = new QPushButton(tr("Filter zurücksetzen"), this);
    m_resetBtn->setToolTip(tr("Suche und alle Filter-Dropdowns auf „Alle“ zurücksetzen"));
    filter->addWidget(m_search, 1);
    filter->addWidget(m_statusFilter);
    filter->addWidget(m_kinematicsFilter);
    filter->addWidget(m_resetBtn);
    layout->addLayout(filter);

    m_model = new PrinterModel(store, this);
    connect(store, &DataStore::dataLoaded, m_model, &PrinterModel::refresh);
    m_view = new QTableView(this);
    m_view->setAlternatingRowColors(true);
    m_view->setModel(m_model);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSortingEnabled(true);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_view);

    connect(m_search, &QLineEdit::textChanged, this, [this]() {
        m_model->setFilter(m_search->text(), m_statusFilter->currentData().toString(),
                           m_kinematicsFilter->currentData().toString());
    });
    connect(m_statusFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setFilter(m_search->text(), m_statusFilter->currentData().toString(),
                           m_kinematicsFilter->currentData().toString());
    });
    connect(m_kinematicsFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setFilter(m_search->text(), m_statusFilter->currentData().toString(),
                           m_kinematicsFilter->currentData().toString());
    });
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        m_search->clear();
        m_statusFilter->setCurrentIndex(0);
        m_kinematicsFilter->setCurrentIndex(0);
    });
    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &idx) {
        if (const Printer *d = m_model->printerAtRow(idx.row())) showDetails(*d);
    });
    connect(m_view, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex idx = m_view->indexAt(pos);
        const Printer *d = idx.isValid() ? m_model->printerAtRow(idx.row()) : nullptr;
        if (!d) return;
        QMenu menu(this);
        menu.addAction(tr("Details anzeigen"), this, [this, d]() { showDetails(*d); });
        QAction *edit = menu.addAction(tr("Stammdaten bearbeiten..."));
        QAction *sim = menu.addAction(tr("PID-Simulation..."));
        connect(edit, &QAction::triggered, this, [this, d]() { editMasterData(*d); });
        connect(sim, &QAction::triggered, this, [this, d]() { openPidSimulation(*d); });
        menu.exec(m_view->viewport()->mapToGlobal(pos));
    });
}

void PrinterWidget::showDetails(const Printer &d)
{
    PrinterDetailDialog dlg(d, m_store, m_loader, this);
    dlg.exec();
}

Printer PrinterWidget::applyRoleRestrictions(const Printer &original, const Printer &edited)
{
    Printer out = edited;

    if ((out.location != original.location || out.status != original.status) && !Auth::mayEditMasterData())
        out.location = original.location, out.status = original.status;
    if ((out.pidKp != original.pidKp || out.pidKi != original.pidKi || out.pidKd != original.pidKd) && !Auth::mayEditPid())
        out.pidKp = original.pidKp, out.pidKi = original.pidKi, out.pidKd = original.pidKd;
    return out;
}

void PrinterWidget::editMasterData(const Printer &d)
{
    if (!Auth::mayEditMasterData() && !Auth::mayEditPid()) {
        QMessageBox::information(this, tr("Rechte"),
            tr("Bearbeitung erfordert mindestens die Rolle „technik“ (PID) bzw. „admin“ (Stammdaten)."));
        return;
    }
    PrinterEditDialog dlg(d, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Printer edited = dlg.editedPrinter();
    if (!dlg.eingabenGueltig()) {
        QMessageBox::warning(this, tr("Ungültige Eingabe"),
            tr("PID-Werte müssen gültige Zahlen sein. Änderung verworfen."));
        return;
    }
    edited.printerId = d.printerId;
    edited.modelId = d.modelId;
    edited.commissionedOn = d.commissionedOn;
    edited.operatingHours = d.operatingHours;
    edited.nozzleMm = d.nozzleMm;
    edited.firmware = d.firmware;

    if ((edited.location != d.location || edited.status != d.status) && !Auth::mayEditMasterData())
        QMessageBox::warning(this, tr("Rechte"),
            tr("Stammdaten (Standort/Status) dürfen nur „admin“ ändern. Änderung verworfen."));
    if ((edited.pidKp != d.pidKp || edited.pidKi != d.pidKi || edited.pidKd != d.pidKd) && !Auth::mayEditPid())
        QMessageBox::warning(this, tr("Rechte"),
            tr("PID-Parameter dürfen nur „technik“/„admin“ ändern. Änderung verworfen."));
    edited = applyRoleRestrictions(d, edited);
    m_store->updatePrinter(edited);
    m_model->resetFilter();
    emit statusMessage(tr("Drucker %1 aktualisiert.").arg(d.printerId));
}

void PrinterWidget::openPidSimulation(const Printer &d)
{

    PidSimDialog dlg(d, m_store, m_loader, this);
    dlg.exec();
}
