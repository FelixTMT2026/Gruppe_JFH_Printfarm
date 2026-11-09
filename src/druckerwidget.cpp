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

DruckerWidget::DruckerWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent)
    : QWidget(parent), m_store(store), m_loader(loader)
{
    auto *layout = new QVBoxLayout(this);

    auto *filter = new QHBoxLayout();
    m_suche = new QLineEdit(this);
    m_suche->setPlaceholderText(tr("Suche nach ID, Modell, Standort..."));
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(tr("Alle Status"), QString());
    m_statusFilter->addItems({QStringLiteral("bereit"), QStringLiteral("druckt"),
                              QStringLiteral("wartung"), QStringLiteral("defekt")});
    m_kinematikFilter = new QComboBox(this);
    m_kinematikFilter->addItem(tr("Alle Kinematiken"), QString());
    m_kinematikFilter->addItems({QStringLiteral("kartesisch"), QStringLiteral("corexy"),
                                 QStringLiteral("delta")});
    filter->addWidget(m_suche, 1);
    filter->addWidget(m_statusFilter);
    filter->addWidget(m_kinematikFilter);
    layout->addLayout(filter);

    m_model = new DruckerModel(store, this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSortingEnabled(true);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_view);

    connect(m_suche, &QLineEdit::textChanged, this, [this]() {
        m_model->setFilter(m_suche->text(), m_statusFilter->currentData().toString(),
                           m_kinematikFilter->currentData().toString());
    });
    connect(m_statusFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setFilter(m_suche->text(), m_statusFilter->currentData().toString(),
                           m_kinematikFilter->currentData().toString());
    });
    connect(m_kinematikFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setFilter(m_suche->text(), m_statusFilter->currentData().toString(),
                           m_kinematikFilter->currentData().toString());
    });
    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &idx) {
        if (const Drucker *d = m_model->druckerAt(idx.row())) oeffneDetail(*d);
    });
    connect(m_view, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex idx = m_view->indexAt(pos);
        const Drucker *d = idx.isValid() ? m_model->druckerAt(idx.row()) : nullptr;
        if (!d) return;
        QMenu menu(this);
        menu.addAction(tr("Details anzeigen"), this, [this, d]() { oeffneDetail(*d); });
        QAction *edit = menu.addAction(tr("Stammdaten bearbeiten..."));
        QAction *sim = menu.addAction(tr("PID-Simulation..."));
        connect(edit, &QAction::triggered, this, [this, d]() { oeffneEdit(*d); });
        connect(sim, &QAction::triggered, this, [this, d]() { oeffneSim(*d); });
        menu.exec(m_view->viewport()->mapToGlobal(pos));
    });
}

void DruckerWidget::oeffneDetail(const Drucker &d)
{
    DruckerDetailDialog dlg(d, m_store, m_loader, this);
    dlg.exec();
}

void DruckerWidget::oeffneEdit(const Drucker &d)
{
    if (!Auth::darfStammdatenAendern() && !Auth::darfPidAendern()) {
        QMessageBox::information(this, tr("Rechte"),
            tr("Bearbeitung erfordert mindestens die Rolle „technik“ (PID) bzw. „admin“ (Stammdaten)."));
        return;
    }
    DruckerEditDialog dlg(d, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Drucker neu = dlg.ergebnis();
    neu.druckerId = d.druckerId;
    neu.modellId = d.modellId;
    neu.inbetriebnahme = d.inbetriebnahme;
    neu.betriebsstunden = d.betriebsstunden;
    neu.dueseMm = d.dueseMm;
    neu.firmware = d.firmware;
    // Rechteprüfung: PID-Felder nur mit technik/admin
    if ((neu.pidKp != d.pidKp || neu.pidKi != d.pidKi || neu.pidKd != d.pidKd)
        && !Auth::darfPidAendern()) {
        QMessageBox::warning(this, tr("Rechte"),
            tr("PID-Parameter dürfen nur „technik“/„admin“ ändern. Änderung verworfen."));
        neu.pidKp = d.pidKp; neu.pidKi = d.pidKi; neu.pidKd = d.pidKd;
    }
    m_store->updateDrucker(neu);
    m_model->resetFilter();
    emit statusMeldung(tr("Drucker %1 aktualisiert.").arg(d.druckerId));
}

void DruckerWidget::oeffneSim(const Drucker &d)
{
    PidSimDialog dlg(d, this);
    dlg.exec();
}
