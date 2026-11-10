#include "materialwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableView>
#include <QHeaderView>
#include <QGroupBox>
#include <QLabel>
#include <QMap>

MaterialWidget::MaterialWidget(DataStore *store, QWidget *parent)
    : QWidget(parent), m_store(store)
{
    auto *layout = new QVBoxLayout(this);
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    auto *matBox = new QGroupBox(tr("Materialien"), this);
    auto *ml = new QVBoxLayout(matBox);
    auto *matView = new QTableView(matBox);
    auto *matModel = new MaterialModel(store, matBox);
    connect(store, &DataStore::datenGeladen, matModel, &MaterialModel::refresh);
    matView->setModel(matModel);
    matView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ml->addWidget(matView);
    splitter->addWidget(matBox);

    auto *modBox = new QGroupBox(tr("Druckermodelle"), this);
    auto *mol = new QVBoxLayout(modBox);
    auto *modView = new QTableView(modBox);
    auto *modModel = new ModellModel(store, modBox);
    connect(store, &DataStore::datenGeladen, modModel, &ModellModel::refresh);
    modView->setModel(modModel);
    modView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mol->addWidget(modView);
    splitter->addWidget(modBox);

    auto *wartBox = new QGroupBox(tr("Wartungen"), this);
    auto *wl = new QVBoxLayout(wartBox);
    auto *wartView = new QTableView(wartBox);
    auto *wartModel = new WartungModel(store, wartBox);
    connect(store, &DataStore::datenGeladen, wartModel, &WartungModel::refresh);
    wartView->setModel(wartModel);
    wartView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    wl->addWidget(wartView);
    splitter->addWidget(wartBox);

    layout->addWidget(splitter);

    // Statistiken (Erweiterung 1): Erfolgsquote je Material, Fehlercode-Häufigkeit
    m_stats = new QLabel(this);
    m_stats->setWordWrap(true);
    layout->addWidget(m_stats);
    connect(store, &DataStore::datenGeladen, this, &MaterialWidget::aktualisiereStats);
    aktualisiereStats();
}

void MaterialWidget::aktualisiereStats()
{
    const DataStore *store = m_store;
    QMap<QString, QPair<int,int>> proMaterial;   // material -> (fertig, erfolgreich)
    QMap<QString, int> fehlercodes;
    double filamentGes = 0, kostenGes = 0;
    for (const auto &a : store->auftraege()) {
        if (a.ergebnis == QLatin1String("ausstehend")) continue;
        auto &p = proMaterial[a.materialId];
        p.first++;
        if (a.ergebnis == QLatin1String("erfolgreich")) { p.second++; filamentGes += a.filamentSollG; }
        if (!a.fehlercode.isEmpty()) fehlercodes[a.fehlercode]++;
        if (const Material *m = store->material(a.materialId))
            if (a.ergebnis == QLatin1String("erfolgreich"))
                kostenGes += a.filamentSollG / 1000.0 * m->preisEurKg;
    }
    QString s = tr("<b>Statistiken:</b> Filamentverbrauch gesamt: %1 kg, Materialkosten gesamt: %2 €<br>")
                    .arg(filamentGes / 1000.0, 0, 'f', 2).arg(kostenGes, 0, 'f', 2);
    s += tr("Erfolgsquote je Material: ");
    for (auto it = proMaterial.begin(); it != proMaterial.end(); ++it)
        s += QStringLiteral("%1 %2%3 ").arg(it.key().toHtmlEscaped())
                 .arg(it.value().first ? 100.0 * it.value().second / it.value().first : 0, 0, 'f', 0)
                 .arg(QLatin1Char('%'));
    s += QStringLiteral("<br>") + tr("Fehlercodes: ");
    for (auto it = fehlercodes.begin(); it != fehlercodes.end(); ++it)
        s += QStringLiteral("%1 (%2) ").arg(it.key().toHtmlEscaped()).arg(it.value());
    m_stats->setText(s);
}
