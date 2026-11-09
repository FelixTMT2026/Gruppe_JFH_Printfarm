#pragma once
// Dialoge: Login, Drucker-Detail, Stammdaten-Bearbeitung, PID-Simulation,
// Berichtsexport.

#include "datatypes.h"
#include "datastore.h"
#include "protocol.h"
#include "auth.h"
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    Auth::Rolle ausgewaehlteRolle() const;
    QString name() const;
private:
    QComboBox *m_rolle;
    QLineEdit *m_name;
};

class DruckerDetailDialog : public QDialog {
    Q_OBJECT
public:
    DruckerDetailDialog(const Drucker &d, const DataStore *store,
                        ProtocolLoader *loader, QWidget *parent = nullptr);
private:
    void zeigeAuftrag(const Auftrag &a);
    const DataStore *m_store;
    ProtocolLoader *m_loader;
    QString m_druckerId;
    class QLabel *m_stammdaten;
    class QTableView *m_auftraegeView;
    class PlotWidget *m_plot;
    class QLabel *m_regelguete;
    class QLabel *m_befunde;
    class AuftragModel *m_model;
};

class DruckerEditDialog : public QDialog {
    Q_OBJECT
public:
    DruckerEditDialog(const Drucker &d, QWidget *parent = nullptr);
    Drucker ergebnis() const;
private:
    QLineEdit *m_standort;
    QComboBox *m_status;
    QLineEdit *m_kp, *m_ki, *m_kd;
};

class PidSimDialog : public QDialog {
    Q_OBJECT
public:
    PidSimDialog(const Drucker &d, QWidget *parent = nullptr);
private:
    QLineEdit *m_kp, *m_ki, *m_kd, *m_tsoll;
    class PlotWidget *m_plot;
    class QLabel *m_ergebnis;
    void simuliere();
};

class BerichtDialog : public QDialog {
    Q_OBJECT
public:
    BerichtDialog(const DataStore *store, QWidget *parent = nullptr);
private:
    const DataStore *m_store;
    class QComboBox *m_typ;
    void exportiere();
};
