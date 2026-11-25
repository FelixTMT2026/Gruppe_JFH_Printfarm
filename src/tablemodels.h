// Qt-Tabellenmodelle fuer Drucker, Auftraege, Materialien, Wartungen, Log.
#pragma once

#include "datatypes.h"
#include "datastore.h"
#include <QAbstractTableModel>

class PrinterModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColModel, ColLocation, ColOperatingHours, ColStatus, ColCount };
    explicit PrinterModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void sort(int column, Qt::SortOrder order) override;
    const Printer *printerAtRow(int row) const;
    void setFilter(const QString &text, const QString &statusFilter, const QString &kinematicsFilter);
    void resetFilter();
    void refresh() { rebuild(); }
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Printer*> m_rows;
    QString m_filterText, m_status, m_kinematics;
};

class JobModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColPrinter, ColPart, ColStart, ColDuration, ColResult, ColError, ColCount };
    explicit JobModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void sort(int column, Qt::SortOrder order) override;
    const Job *jobAtRow(int row) const;
    void setFilter(const QString &text, const QString &resultFilter);
    void setPrinterOnly(const QString &printerId);
    void resetFilter();
    void refresh() { rebuild(); }
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Job*> m_rows;
    QString m_filterText, m_resultLabel, m_printerId;
};

class MaterialModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColBez, ColNozzle, ColBed, ColDensity, ColPrice, ColFan, ColWeeklyConsumption, ColCount };
    explicit MaterialModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void sort(int column, Qt::SortOrder order) override;
public slots:
    void refresh() { rebuild(); }
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Material*> m_rows;
};

class MaintenanceModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColPrinter, ColDate, ColArt, ColDuration, ColNote, ColCount };
    explicit MaintenanceModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void setPrinterOnly(const QString &printerId);
    void refresh() { rebuild(); }
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Maintenance*> m_rows;
    QString m_printerId;
};

class LogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColTime = 0, ColCategory, ColJob, ColMessage, ColCount };
    explicit LogModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void setCategoryFilter(int category);
    void refresh();
private:
    void rebuildIndex();
    const DataStore *m_store;
    QList<int> m_index;
    int m_categoryFilter = -1;
};
