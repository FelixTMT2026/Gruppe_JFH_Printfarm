#pragma once
// Tab 1 "Druckerliste": Suche/Filter, Kontextmenü, Detail- und Bearbeitungsdialog.

#include "datastore.h"
#include "protocol.h"
#include "tablemodels.h"
#include <QWidget>

class DruckerWidget : public QWidget {
    Q_OBJECT
public:
    explicit DruckerWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent = nullptr);
    DruckerModel *model() const { return m_model; }
signals:
    void statusMeldung(const QString &text);
private:
    void oeffneDetail(const Drucker &d);
    void oeffneEdit(const Drucker &d);
    void oeffneSim(const Drucker &d);
    DataStore *m_store;
    ProtocolLoader *m_loader;
    DruckerModel *m_model;
    class QTableView *m_view;
    class QLineEdit *m_suche;
    class QComboBox *m_statusFilter;
    class QComboBox *m_kinematikFilter;
};
