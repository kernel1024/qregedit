#ifndef CSAMGROUPSMODEL_H
#define CSAMGROUPSMODEL_H

#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QTreeView>
#include <QTableView>

class CSAMGroupsModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_DISABLE_COPY(CSAMGroupsModel)

private:
    int hive_num { -1 };
    int groups_count { 0 };

public:
    explicit CSAMGroupsModel(QObject *parent = nullptr);
    ~CSAMGroupsModel() override;

    void keyChanged(const QModelIndex& key, QTreeView *view);

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class CSAMUsersModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(CSAMUsersModel)

private:
    int hive_num { -1 };
    int val_count { 0 };

public:
    explicit CSAMUsersModel(QObject *parent = nullptr);
    ~CSAMUsersModel() override;

    void keyChanged(const QModelIndex& key, QTableView *view);
    int getUserRID(const QModelIndex& index) const;
    int getHiveIdx() const { return hive_num; }

protected:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};

#endif // CSAMGROUPSMODEL_H
