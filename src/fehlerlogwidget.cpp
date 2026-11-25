// Implementierung siehe fehlerlogwidget.h.

#include "fehlerlogwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>

ErrorLogWidget::ErrorLogWidget(DataStore *store, QWidget *parent)
    : QWidget(parent), m_store(store)
{
    auto *layout = new QVBoxLayout(this);
    auto *filter = new QHBoxLayout();
    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItem(tr("Alle Kategorien"), -1);
    m_categoryFilter->addItem(tr("Importfehler"), int(LogEntry::Import));
    m_categoryFilter->addItem(tr("Validierungsfehler"), int(LogEntry::Validation));
    m_categoryFilter->addItem(tr("Dateifehler"), int(LogEntry::File));
    m_categoryFilter->addItem(tr("Protokollfehler"), int(LogEntry::PrintLog));
    m_categoryFilter->addItem(tr("System"), int(LogEntry::System));
    m_resetBtn = new QPushButton(tr("Filter zurücksetzen"), this);
    m_resetBtn->setToolTip(tr("Kategorie-Filter auf „Alle Kategorien“ zurücksetzen"));
    filter->addWidget(m_categoryFilter);
    filter->addWidget(m_resetBtn);
    filter->addStretch();
    layout->addLayout(filter);

    m_model = new LogModel(store, this);
    auto *view = new QTableView(this);
    view->setAlternatingRowColors(true);
    view->setModel(m_model);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(view);

    connect(m_categoryFilter, &QComboBox::currentIndexChanged, this, [this]() {
        m_model->setCategoryFilter(m_categoryFilter->currentData().toInt());
    });
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        m_categoryFilter->setCurrentIndex(0);
    });
    connect(store, &DataStore::logChanged, m_model, &LogModel::refresh);
    connect(store, &DataStore::dataLoaded, m_model, &LogModel::refresh);
}
