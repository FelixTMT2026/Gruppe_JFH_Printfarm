#pragma once
// Tab 3 "Material": Materialien, Modelle, Wartungen und Statistiken.

#include "datastore.h"
#include "tablemodels.h"
#include <QWidget>

class MaterialWidget : public QWidget {
    Q_OBJECT
public:
    explicit MaterialWidget(DataStore *store, QWidget *parent = nullptr);
private:
    DataStore *m_store;
    class QLabel *m_stats;
};
