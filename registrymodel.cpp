#include <QIcon>
#include "registrymodel.h"
#include "global.h"
#include <QDebug>

CRegistryModel::CRegistryModel()
{
    cgl->reg->treeModel = this;
}

void CRegistryModel::beginInsertRows(const QModelIndex &parent, int first, int last)
{
    QAbstractItemModel::beginInsertRows(parent, first, last);
}

void CRegistryModel::endInsertRows()
{
    QAbstractItemModel::endInsertRows();
}

QModelIndex CRegistryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid()) {
        if (row<0 || row>=cgl->reg->getHivesCount())
            return QModelIndex();
        struct hive* h = cgl->reg->getHivePtr(row);
        return createIndex(row, column, cgl->reg->getKeyPtr(h,h->rootofs + 4));
    } else {
        int hive = cgl->reg->getHive((struct nk_key*)parent.internalPointer());
        if (hive==-1)
            return QModelIndex();

        struct hive* h = cgl->reg->getHivePtr(hive);

        QList<int> keys = cgl->reg->listKeysOfs(h,(struct nk_key*)parent.internalPointer());
        if (row>=0 && row<keys.count())
            return createIndex(row, column, cgl->reg->getKeyPtr(h,keys.at(row)));
        else
            return QModelIndex();
    }
}

QModelIndex CRegistryModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    struct nk_key* ck;
    struct hive* h;
    int hive;
    if (!cgl->reg->keyPrepare(child.internalPointer(),h,hive,ck))
        return QModelIndex();

    // Child is hive - no parent
    if (cgl->reg->getKeyOfs(h,ck)==(h->rootofs + 4))
        return QModelIndex();

    // Child is key
    struct nk_key* pk = cgl->reg->getKeyPtr(h,ck->ofs_parent + 0x1004);
    if (!cgl->reg->checkKey(pk))
        return QModelIndex();

    // Parent is hive - no grand parent, row = hive number
    if (cgl->reg->getKeyOfs(h,pk)==(h->rootofs+4))
        return createIndex(hive,0,pk);

    // Parent is also key, row - index in grandparent's child list
    struct nk_key* gpk = cgl->reg->getKeyPtr(h,pk->ofs_parent + 0x1004);
    if (!cgl->reg->checkKey(gpk))
        return QModelIndex();

    QList<int> pkeys = cgl->reg->listKeysOfs(h,gpk);
    int row = pkeys.indexOf(cgl->reg->getKeyOfs(h,pk));
    if (row<0 || row>=pkeys.count())
        qFatal("search for parent index in grandparent's child list failure");

    return createIndex(row,0,pk);
}

int CRegistryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return cgl->reg->getHivesCount();

    struct nk_key* k;
    struct hive* h;
    int hive;
    if (!cgl->reg->keyPrepare(parent.internalPointer(),h,hive,k))
        return 0;

    return k->no_subkeys;
}

int CRegistryModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant CRegistryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    struct nk_key* k;
    struct hive* h;
    int hive;
    if (!cgl->reg->keyPrepare(index.internalPointer(),h,hive,k))
        return QVariant();

    if (role == Qt::DisplayRole) {
        return cgl->reg->getKeyName(h, k);

    } else if (role == Qt::ToolTipRole) {
        QString s = cgl->reg->getKeyTooltip(h, k);
        if (!s.isEmpty()) return s;

    } else if (role == Qt::DecorationRole) {
        return QIcon(":/icons/folder");
    } else if (role == Qt::StatusTipRole) {
        QString s = cgl->reg->getKeyFullPath(h, k);
        if (!s.isEmpty()) return s;
    }

    return QVariant();
}

Qt::ItemFlags CRegistryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}
