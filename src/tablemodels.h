#pragma once
// QAbstractTableModel-Implementierungen für die Listenansichten.

#include "datatypes.h"
#include "datastore.h"
#include <QAbstractTableModel>

class DruckerModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColModell, ColStandort, ColBetriebsstunden, ColStatus, ColCount };
    explicit DruckerModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    const Drucker *druckerAt(int row) const;
    void setFilter(const QString &text, const QString &statusFilter, const QString &kinematikFilter);
    void resetFilter();
    void refresh() { rebuild(); }   // nach (Neu-)Laden der Daten
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Drucker*> m_rows;
    QString m_text, m_status, m_kinematik;
};

class AuftragModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColDrucker, ColBauteil, ColStart, ColDauer, ColErgebnis, ColFehler, ColCount };
    explicit AuftragModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    const Auftrag *auftragAt(int row) const;
    void setFilter(const QString &text, const QString &ergebnisFilter); // text: id/drucker/bauteil
    void setNurDrucker(const QString &druckerId);   // leer = alle
    void resetFilter();
    void refresh() { rebuild(); }
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Auftrag*> m_rows;
    QString m_text, m_ergebnis, m_druckerId;
};

class MaterialModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColBez, ColDuese, ColBett, ColDichte, ColPreis, ColLuefter, ColCount };
    explicit MaterialModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
public slots:
    void refresh() { beginResetModel(); endResetModel(); }
private:
    const DataStore *m_store;
};

class ModellModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColHersteller, ColBez, ColKinematik, ColBauraum, ColMaxDuese, ColMaxBett, ColCount };
    explicit ModellModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
public slots:
    void refresh() { beginResetModel(); endResetModel(); }
private:
    const DataStore *m_store;
};

class WartungModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColId = 0, ColDrucker, ColDatum, ColArt, ColDauer, ColBemerkung, ColCount };
    explicit WartungModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void setNurDrucker(const QString &druckerId);
    void refresh() { rebuild(); }
private:
    void rebuild();
    const DataStore *m_store;
    QList<const Wartung*> m_rows;
    QString m_druckerId;
};

class LogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum { ColZeit = 0, ColKategorie, ColAuftrag, ColMeldung, ColCount };
    explicit LogModel(const DataStore *store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &p = {}) const override;
    int columnCount(const QModelIndex &p = {}) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    QVariant headerData(int s, Qt::Orientation o, int role) const override;
    void setKategorieFilter(int kat);   // -1 = alle
    void refresh();
private:
    const DataStore *m_store;
    int m_katFilter = -1;
};
