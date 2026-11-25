// Tab 1 (Druckerliste): Suche/Filter/Sortierung, Kontextmenue, Rechte.
#pragma once

#include "datastore.h"
#include "protocol.h"
#include "tablemodels.h"
#include <QWidget>

class PrinterWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrinterWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent = nullptr);
    PrinterModel *model() const { return m_model; }

    static Printer applyRoleRestrictions(const Printer &original, const Printer &edited);
signals:
    void statusMessage(const QString &text);
private:
    void showDetails(const Printer &d);
    void editMasterData(const Printer &d);
    void openPidSimulation(const Printer &d);
    DataStore *m_store;
    ProtocolLoader *m_loader;
    PrinterModel *m_model;
    class QTableView *m_view;
    class QLineEdit *m_search;
    class QComboBox *m_statusFilter;
    class QComboBox *m_kinematicsFilter;
    class QPushButton *m_resetBtn;
};
