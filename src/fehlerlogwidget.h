// Tab 4 (Fehlerlog): Log-Tabelle mit Kategoriefilter.
#pragma once

#include "datastore.h"
#include "tablemodels.h"
#include <QWidget>

class ErrorLogWidget : public QWidget {
    Q_OBJECT
public:
    explicit ErrorLogWidget(DataStore *store, QWidget *parent = nullptr);
private:
    DataStore *m_store;
    LogModel *m_model;
    class QComboBox *m_categoryFilter;
    class QPushButton *m_resetBtn;
};
