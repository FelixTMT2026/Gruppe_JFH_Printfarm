#pragma once
// Tab 2 "Gedruckte Teile": alle Aufträge (erfolgreich, abgebrochen,
// fehlgeschlagen, geplant), Verlaufsanzeige mit Regelgüte, Kostenrechnung,
// Auftragsplanung mit Gantt-Ansicht.

#include "datastore.h"
#include "protocol.h"
#include "tablemodels.h"
#include "analyse.h"
#include <QWidget>

class TeileWidget : public QWidget {
    Q_OBJECT
public:
    explicit TeileWidget(DataStore *store, ProtocolLoader *loader, QWidget *parent = nullptr);
signals:
    void statusMeldung(const QString &text);
private:
    void zeigeAuftrag(const Auftrag &a);
    void planeAuftraege();
    void initialAuswahl();   // nach Laden: nach Zeit sortieren + letzten fertigen Druck wählen
    DataStore *m_store;
    ProtocolLoader *m_loader;
    AuftragModel *m_model;
    class QTableView *m_view;
    class QLineEdit *m_suche;
    class QComboBox *m_ergebnisFilter;
    class QPushButton *m_resetBtn;
    class PlotWidget *m_plot;
    class QLabel *m_info;
    class QLabel *m_kosten;
    class QPushButton *m_planBtn;
    QString m_aktuellerAuftrag;
};
