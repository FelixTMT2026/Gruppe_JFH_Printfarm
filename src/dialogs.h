// Alle Dialoge: Login, Drucker-Detail/-Bearbeitung, PID-Simulation, Berichtsexport.
#pragma once

#include "datatypes.h"
#include "datastore.h"
#include "protocol.h"
#include "pidsim.h"
#include "auth.h"
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    Auth::Role selectedRole() const;
    QString name() const;
    QString password() const;
private:
    QComboBox *m_role;
    QLineEdit *m_name;
    QLineEdit *m_password;
};

class PrinterDetailDialog : public QDialog {
    Q_OBJECT
public:
    PrinterDetailDialog(const Printer &d, const DataStore *store,
                        ProtocolLoader *loader, QWidget *parent = nullptr);
private:
    void showJob(const Job &a);
    const DataStore *m_store;
    ProtocolLoader *m_loader;
    QString m_printerId;
    class QLabel *m_masterDataLabel;
    class QTableView *m_jobsView;
    class QTableView *m_maintenancesView;
    class PlotWidget *m_plot;
    class QLabel *m_qualityLabel;
    class QLabel *m_findingsLabel;
    class JobModel *m_model;
    class MaintenanceModel *m_maintenanceModel;
};

class PrinterEditDialog : public QDialog {
    Q_OBJECT
public:
    PrinterEditDialog(const Printer &d, QWidget *parent = nullptr);
    Printer editedPrinter() const;
    bool eingabenGueltig() const { return m_gueltig; }
private:
    QString m_printerId;
    QLineEdit *m_locationEdit;
    QComboBox *m_status;
    QLineEdit *m_kp, *m_ki, *m_kd;
    bool m_gueltig = true;
};

class PidSimDialog : public QDialog {
    Q_OBJECT
public:

    PidSimDialog(const Printer &d, const DataStore *store,
                 ProtocolLoader *loader, QWidget *parent = nullptr);
private:
    QString m_modelId;
    QString m_printerId;
    const DataStore *m_store;
    ProtocolLoader *m_loader;
    QLineEdit *m_kp, *m_ki, *m_kd, *m_targetTemp;
    class QCheckBox *m_disturbanceCheck, *m_noiseCheck, *m_fanCheck, *m_fit;
    class PlotWidget *m_plot;
    class QLabel *m_resultLabel;
    class QListWidget *m_rankingList;
    void startSimulation();

    PidSim::Options resolvePlantOptions(QString &info) const;

    void displayRanking(const QList<PidSim::RankEntry> &ranking, const QString &plantInfo);
};

class ReportDialog : public QDialog {
    Q_OBJECT
public:
    ReportDialog(const DataStore *store, QWidget *parent = nullptr);
private:
    const DataStore *m_store;
    class QComboBox *m_type;
    void exportReport();
};
