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

CValuesModel::CValuesModel()
{
    cgl->reg->valuesModel = this;
    key_ofs = -1;
    hive = -1;
}

void CValuesModel::keyChanged(const QModelIndex &key)
{
    if (!key.isValid()) return;

    struct nk_key* ck;
    struct hive* h;
    if (!cgl->reg->keyPrepare(key.internalPointer(),h,hive,ck)) {
        hive = -1;
        key_ofs = -1;
        return;
    }
    key_ofs = cgl->reg->getKeyOfs(h,ck);
}

int CValuesModel::rowCount(const QModelIndex &parent) const
{
    if (hive<0 || key_ofs<0)
        return 0;

    struct hive* h = cgl->reg->getHivePtr(hive);
    struct nk_key* k = cgl->reg->getKeyPtr(h, key_ofs);

    QList<CValue> vl = cgl->reg->listValues(h, k);

    return vl.count();
}

int CValuesModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant CValuesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || hive<0 || key_ofs<0)
        return QVariant();

    struct hive* h = cgl->reg->getHivePtr(hive);
    struct nk_key* k = cgl->reg->getKeyPtr(h, key_ofs);

    QList<CValue> vl = cgl->reg->listValues(h, k);

    int row = index.row();
    int col = index.column();

    if (role == Qt::DisplayRole && row>=0 && row<vl.count()) {
        if (col==0) {
            return vl.at(row).name;
        } else if (col==1) {
            switch (vl.at(row).type) {
                case REG_NONE: return QString("REG_NONE");
                case REG_SZ: return QString("REG_SZ");
                case REG_EXPAND_SZ: return QString("REG_EXPAND_SZ");
                case REG_BINARY: return QString("REG_BINARY");
                case REG_DWORD: return QString("REG_DWORD");
                case REG_DWORD_BIG_ENDIAN: return QString("REG_DWORD_BIG_ENDIAN");
                case REG_LINK: return QString("REG_LINK");
                case REG_MULTI_SZ: return QString("REG_MULTI_SZ");
                case REG_RESOURCE_LIST: return QString("REG_RESOURCE_LIST");
                case REG_FULL_RESOURCE_DESCRIPTOR: return QString("REG_FULL_RESOURCE_DESCRIPTOR");
                case REG_RESOURCE_REQUIREMENTS_LIST: return QString("REG_RESOURCE_REQUIREMENTS_LIST");
                case REG_QWORD: return QString("REG_QWORD");
                default: return QVariant();
            }
        }

        return cgl->reg->getKeyName(h, k);

    } else if (role == Qt::DecorationRole) {
        return QIcon(":/icons/folder");
    }

    return QVariant();
}

Qt::ItemFlags CValuesModel::flags(const QModelIndex &index) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant CValuesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)

    if (role == Qt::DisplayRole && section>=0 && section<vl.count()) {
        switch (section) {
            case 0: return QString("Name");
            case 1: return QString("Type");
            case 2: return QString("Value");
            default: return QVariant();
        }
    }
    return QVariant();
}


