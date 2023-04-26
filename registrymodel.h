#ifndef REGISTRYMODEL_H
#define REGISTRYMODEL_H

#include <QAbstractItemModel>
#include <QPointer>
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
    Q_DISABLE_COPY(CRegistryModel)

private:
    QModelIndex getKeyIndex(struct hive *hdesc, struct nk_key *key);

public:
    QPointer<CFinder> finder;

    explicit CRegistryModel(QObject *parent = nullptr);
    ~CRegistryModel() override;

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
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void keyFound(const QModelIndex &index, const QString &value);
    void destroyFinder();

private Q_SLOTS:
    void finderKeyFound(quintptr hdesc, quintptr key, const QString &value);

};

class CValuesModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(CValuesModel)

private:
    int val_count { 0 };
    int key_ofs { -1 };
    int hive_num { -1 };
    void *key_ptr { nullptr };
    QString m_keyName;

public:
    explicit CValuesModel(QObject *parent = nullptr);
    ~CValuesModel() override;

    void keyChanged(const QModelIndex& key);
    QString getCurrentKeyName() { return m_keyName; }
    void reloadKey(void *newKey = nullptr);

    bool renameValue(const QModelIndex &idx, const QString& name);
    bool deleteValue(const QModelIndex &idx);

    QString getValueName(const QModelIndex &idx) const;
    CValue getValue(const QModelIndex &idx) const;
    QModelIndex getValueIdx(const QString& name) const;

    bool setValue(const QModelIndex& idx, const CValue& value);

    bool createValue(const CValue& value);

protected:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

Q_SIGNALS:
    void valuesReloaded();

};

#endif // REGISTRYMODEL_H
