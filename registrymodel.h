#ifndef REGISTRYMODEL_H
#define REGISTRYMODEL_H

#include <QAbstractItemModel>
#include <QTableView>
#include <QVector>
#include <QString>
#include "finder.h"

class CValue;
struct hive;
struct nk_key;

class CRegistryModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    QModelIndex getKeyIndex(struct hive *hdesc, struct nk_key *key);

public:
    CFinder* finder;

    CRegistryModel();

    void beginInsertRows(const QModelIndex &parent, int first, int last);
    void endInsertRows();
    void beginRemoveRows(const QModelIndex &parent, int first, int last);
    void endRemoveRows();

    int getHiveIdx(const QModelIndex& index);
    QString getKeyName(const QModelIndex &index) const;
    bool createKey(const QModelIndex &parent, const QString& name);
    void deleteKey(const QModelIndex &idx);

    bool exportKey(const QModelIndex &idx, const QString& filename);

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
    void keyFound(const QModelIndex &index, const QString &value);

private slots:
    void finderKeyFound(struct hive *hdesc, struct nk_key *key, const QString &value);

};

class CValuesModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    int val_count;
    int key_ofs;
    int hive_num;
    QString m_keyName;

public:
    CValuesModel();

    void keyChanged(const QModelIndex& key, QTableView *table);
    QString getCurrentKeyName() { return m_keyName; }

    bool renameValue(const QModelIndex &idx, const QString& name);
    bool deleteValue(const QModelIndex &idx);

    QString getValueName(const QModelIndex& idx);
    CValue getValue(const QModelIndex& idx);
    QModelIndex getValueIdx(const QString& name) const;

    bool setValue(const QModelIndex& idx, const CValue& value);

    bool createValue(const CValue& value);

protected:
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

#endif // REGISTRYMODEL_H
