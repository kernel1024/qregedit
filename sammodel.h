#ifndef CSAMGROUPSMODEL_H
#define CSAMGROUPSMODEL_H

#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QTreeView>
#include <QTableView>

class CSAMGroupsModel : public QAbstractItemModel
{
    Q_OBJECT
private:
    int hive_num;
    int groups_count;

public:
    explicit CSAMGroupsModel();

    void keyChanged(const QModelIndex& key, QTreeView *view);

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

class CSAMUsersModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    int hive_num;
    int val_count;

public:
    explicit CSAMUsersModel();

    void keyChanged(const QModelIndex& key, QTableView *view);

protected:
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // CSAMGROUPSMODEL_H
