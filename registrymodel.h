#ifndef REGISTRYMODEL_H
#define REGISTRYMODEL_H

#include <QAbstractItemModel>
#include <QTableView>
#include <QVector>
#include <QString>

class CRegistryModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    CRegistryModel();

    void beginInsertRows(const QModelIndex &parent, int first, int last);
    void endInsertRows();

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

};

class CValuesModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    int key_ofs;
    int hive;

public:
    CValuesModel();

    void keyChanged(const QModelIndex& key, QTableView *table);

protected:
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // REGISTRYMODEL_H
