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

TeileWidget::TeileWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent)
    : QWidget(parent), m_store(store), m_loader(loader), m_aktuellerAuftrag(QString())
{
    auto *layout = new QVBoxLayout(this);

    auto *filter = new QHBoxLayout();
    m_suche = new QLineEdit(this);
    m_suche->setPlaceholderText(tr("Suche nach Auftrag, Drucker, Bauteil..."));
    m_suche->setClearButtonEnabled(true);
    m_ergebnisFilter = new QComboBox(this);
    m_ergebnisFilter->addItem(tr("Alle Ergebnisse"), QString());
    m_ergebnisFilter->addItem(QStringLiteral("erfolgreich"), QStringLiteral("erfolgreich"));
    m_ergebnisFilter->addItem(QStringLiteral("abgebrochen"), QStringLiteral("abgebrochen"));
    m_ergebnisFilter->addItem(QStringLiteral("fehlgeschlagen"), QStringLiteral("fehlgeschlagen"));
    m_ergebnisFilter->addItem(QStringLiteral("ausstehend"), QStringLiteral("ausstehend"));
    m_planBtn = new QPushButton(tr("Belegungsplan (Gantt)"), this);
    m_planBtn->setToolTip(tr("Geplante Aufträge greedy auf geeignete Drucker verteilen (nur Admin)"));
    // Belegungsplan nur für Admin sichtbar
    m_planBtn->setVisible(Auth::rolle() == Auth::Admin);
    filter->addWidget(m_suche, 1);
    filter->addWidget(m_ergebnisFilter);
    filter->addWidget(m_planBtn);
    layout->addLayout(filter);

    // oben: Protokollansicht auf voller Breite
    auto *detailBox = new QGroupBox(tr("Protokoll / Verlauf / Auswertung"), this);
    auto *dl = new QVBoxLayout(detailBox);
    m_info = new QLabel(detailBox);
    m_info->setWordWrap(true);
    m_plot = new PlotWidget(detailBox);
    m_kosten = new QLabel(detailBox);
    m_kosten->setWordWrap(true);
    dl->addWidget(m_info);
    dl->addWidget(m_plot, 1);
    dl->addWidget(m_kosten);
    layout->addWidget(detailBox, 3);

    // darunter: Jobliste
    m_model = new AuftragModel(store, this);
    connect(store, &DataStore::datenGeladen, m_model, &AuftragModel::refresh);
    connect(store, &DataStore::datenGeladen, this, &TeileWidget::initialAuswahl);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSortingEnabled(true);
    layout->addWidget(m_view, 2);

    connect(m_suche, &QLineEdit::textChanged, this, [this]() {
        m_model->setFilter(m_suche->text(), m_ergebnisFilter->currentData().toString());
    });
    connect(m_ergebnisFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setFilter(m_suche->text(), m_ergebnisFilter->currentData().toString());
    });
    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &idx) {
        if (const Auftrag *a = m_model->auftragAt(idx.row())) zeigeAuftrag(*a);
    });
    connect(m_planBtn, &QPushButton::clicked, this, &TeileWidget::planeAuftraege);
    connect(m_loader, &ProtocolLoader::fertig, this, [this](const QString &id) {
        emit statusMeldung(tr("Protokoll %1 im Hintergrund geladen (Cache).").arg(id));
        // wenn das gerade der angezeigte Auftrag ist: Ansicht aktualisieren
        if (id == m_aktuellerAuftrag) {
            auto *tabs = findChild<QTabWidget*>();
            Q_UNUSED(tabs);
            for (const auto &a : m_store->auftraege())
                if (a.auftragId == id) { zeigeAuftrag(a); break; }
        }
    });
    connect(m_loader, &ProtocolLoader::fehler, this, [this](const QString &id, const QString &f) {
        m_store->addLog(LogEintrag::Protokoll, f, id);
        emit statusMeldung(tr("Protokollfehler %1: %2").arg(id, f));
    });
    m_plot->leeren();
    // Daten bereits geladen (z. B. nach Login/Sprachwechsel mit UI-Neuaufbau)?
    if (!store->auftraege().isEmpty())
        initialAuswahl();
}

void TeileWidget::zeigeAuftrag(const Auftrag &a)
{
    m_aktuellerAuftrag = a.auftragId;
    const Material *mat = m_store->material(a.materialId);
    m_info->setText(tr("<b>%1</b> - %2 auf %3 | Material: %4 | Soll: %5 min, %6 g | Ergebnis: %7%8")
        .arg(a.auftragId, a.bauteil, a.druckerId, mat ? mat->bezeichnung : a.materialId)
        .arg(a.dauerSollMin).arg(a.filamentSollG).arg(a.ergebnis)
        .arg(a.fehlercode.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(a.fehlercode)));

    const auto k = Kostenrechnung::auftrag(a, mat);
    m_kosten->setText(tr("Kosten: Material %1 € + Maschine %2 € = %3 €")
        .arg(k.materialEur, 0, 'f', 2).arg(k.maschinenEur, 0, 'f', 2).arg(k.gesamtEur, 0, 'f', 2));

    if (a.ergebnis == QLatin1String("ausstehend")) {
        m_plot->leeren();
        return;
    }
    if (!m_loader->cached(a.auftragId))
        m_loader->loadAsync(a.auftragId);   // Hintergrund laden, Anzeige bei fertigem Signal erneut
    const Protokoll p = m_loader->load(a.auftragId);
    if (!p.valid) {
        m_plot->leeren();
        m_info->setText(m_info->text() + tr("<br><i>Kein Protokoll verfügbar: %1</i>").arg(p.fehler));
        return;
    }
    m_plot->zeigeProtokoll(p);
    const Regelguete rg = RegelgueteRechner::werteAus(p.aufheizen, p.druck, p.dueseSollC);
    if (rg.verfuegbar) {
        m_kosten->setText(m_kosten->text() + tr("<br>Regelgüte: tr=%1 s, Überschwingen=%2 K (%3 %), Ausregelzeit=%4, Ø-Abweichung=%5 K, max=%6 K")
            .arg(rg.anstiegszeitS, 0, 'f', 1).arg(rg.ueberschwingenK, 0, 'f', 2)
            .arg(rg.ueberschwingenProzent, 0, 'f', 1)
            .arg(rg.ausgeregelt ? QString::number(rg.ausregelzeitS, 'f', 0) + " s" : tr("nicht ausgeregelt"))
            .arg(rg.mittlereAbweichungK, 0, 'f', 2).arg(rg.maximaleAbweichungK, 0, 'f', 2));
        const auto bef = Fehlererkennung::analysiere(p, rg, mat);
        if (bef.schlechtEingestellt || !bef.fehlerbilder.isEmpty()) {
            QStringList fb;
            if (bef.schlechtEingestellt) fb.append(tr("Regler schlecht eingestellt"));
            fb.append(bef.fehlerbilder);
            m_store->addLog(LogEintrag::Protokoll,
                tr("Auftrag %1: %2").arg(a.auftragId, fb.join(QStringLiteral("; "))), a.auftragId);
        }
    }
}

void TeileWidget::initialAuswahl()
{
    // Default-Sortierung: nach Startzeit absteigend (neueste zuerst)
    m_model->sort(AuftragModel::ColStart, Qt::DescendingOrder);
    // letzten beendeten Druck auswählen (max. Endzeit unter den fertigen Aufträgen)
    const Auftrag *last = nullptr;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        const Auftrag *a = m_model->auftragAt(i);
        if (!a || a->ergebnis == QLatin1String("ausstehend") || !a->ende.isValid())
            continue;
        if (!last || a->ende > last->ende)
            last = a;
    }
    if (!last)
        return;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (m_model->auftragAt(i) == last) {
            m_view->setCurrentIndex(m_model->index(i, 0));
            m_view->selectRow(i);
            zeigeAuftrag(*last);   // Protokoll + Plot + Regelgüte sofort anzeigen
            break;
        }
    }
}

void TeileWidget::planeAuftraege()
{
    // Belegungsplan nur als Admin
    if (Auth::rolle() != Auth::Admin) {
        QMessageBox::information(this, tr("Rechte"),
            tr("Der Belegungsplan ist nur für die Rolle „admin“ verfügbar."));
        return;
    }
    QList<Auftrag> ausstehend;
    for (const auto &a : m_store->auftraege())
        if (a.ergebnis == QLatin1String("ausstehend"))
            ausstehend.append(a);
    const auto zuweisungen = Planung::greedy(ausstehend, m_store->drucker(), *m_store,
                                             QDateTime::currentDateTime());
    QStringList druckerIds;
    for (const auto &z : zuweisungen)
        if (!druckerIds.contains(z.druckerId)) druckerIds.append(z.druckerId);
    m_store->addLog(LogEintrag::System,
        tr("Greedy-Planung: %1 geplante Aufträge auf %2 Zuweisungen verteilt")
            .arg(ausstehend.size()).arg(zuweisungen.size()));
    emit statusMeldung(tr("Planung: %1 Zuweisungen (siehe Belegungsplan).").arg(zuweisungen.size()));

    // Belegungsplan als Popup-Dialog
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Belegungsplan (Gantt) - geplante Aufträge"));
    dlg.resize(900, 500);
    auto *dl = new QVBoxLayout(&dlg);
    auto *gantt = new GanttWidget(&dlg);
    gantt->setPlanung(zuweisungen, druckerIds);
    dl->addWidget(gantt);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    dl->addWidget(btns);
    dlg.exec();
}
