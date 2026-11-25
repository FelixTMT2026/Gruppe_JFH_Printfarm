// Tab 2 (Gedruckte Teile): Auftragsliste, Protokoll-Plot, Auswertung, Belegungsplan.
#pragma once

#include "datastore.h"
#include "protocol.h"
#include "tablemodels.h"
#include "analyse.h"
#include <QWidget>

class PartsWidget : public QWidget {
    Q_OBJECT
public:
    explicit PartsWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent = nullptr);
signals:
    void statusMessage(const QString &text);
private:
    void showJob(const Job &a);
    void showSchedule();
    void initialAuswahl();
    DataStore *m_store;
    ProtocolLoader *m_loader;
    JobModel *m_model;
    class QTableView *m_view;
    class QLineEdit *m_search;
    class QComboBox *m_resultFilter;
    class QPushButton *m_resetBtn;
    class PlotWidget *m_plot;
    class QLabel *m_info;
    class QLabel *m_costLabel;
    class QPushButton *m_planBtn;
    QString m_currentJob;
};
