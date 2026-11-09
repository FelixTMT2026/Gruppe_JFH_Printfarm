#pragma once
// Tab 4 "Fehlerlog": Import-, Validierungs-, Datei- und Protokollfehler
// unterscheidbar nach Kategorie, mit Filter.

#include "datastore.h"
#include "tablemodels.h"
#include <QWidget>

class FehlerlogWidget : public QWidget {
    Q_OBJECT
public:
    explicit FehlerlogWidget(DataStore *store, QWidget *parent = nullptr);
private:
    DataStore *m_store;
    LogModel *m_model;
    class QComboBox *m_katFilter;
};
