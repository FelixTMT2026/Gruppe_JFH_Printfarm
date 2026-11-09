#include "fehlerlogwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTableView>
#include <QHeaderView>

FehlerlogWidget::FehlerlogWidget(DataStore *store, QWidget *parent)
    : QWidget(parent), m_store(store)
{
    auto *layout = new QVBoxLayout(this);
    auto *filter = new QHBoxLayout();
    m_katFilter = new QComboBox(this);
    m_katFilter->addItem(tr("Alle Kategorien"), -1);
    m_katFilter->addItem(tr("Importfehler"), int(LogEintrag::Import));
    m_katFilter->addItem(tr("Validierungsfehler"), int(LogEintrag::Validierung));
    m_katFilter->addItem(tr("Dateifehler"), int(LogEintrag::Datei));
    m_katFilter->addItem(tr("Protokollfehler"), int(LogEintrag::Protokoll));
    m_katFilter->addItem(tr("System"), int(LogEintrag::System));
    filter->addWidget(m_katFilter);
    filter->addStretch();
    layout->addLayout(filter);

    m_model = new LogModel(store, this);
    auto *view = new QTableView(this);
    view->setModel(m_model);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(view);

    connect(m_katFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setKategorieFilter(m_katFilter->currentData().toInt());
    });
    connect(store, &DataStore::logChanged, m_model, &LogModel::refresh);
    connect(store, &DataStore::datenGeladen, m_model, &LogModel::refresh);
}
