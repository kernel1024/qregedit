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

void CRegistryModel::beginRemoveRows(const QModelIndex &parent, int first, int last)
{
    QAbstractItemModel::beginRemoveRows(parent, first, last);
}

void CRegistryModel::endRemoveRows()
{
    QAbstractItemModel::endRemoveRows();
}

int CRegistryModel::getHiveIdx(const QModelIndex &index)
{
    if (!index.isValid()) return -1;

    return cgl->reg->getHive((struct nk_key*)(index.internalPointer()));
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
    Q_UNUSED(parent)

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

void CValuesModel::keyChanged(const QModelIndex &key, QTableView* table)
{
    if (hive>=0 && key_ofs>=0) {
        struct hive* h = cgl->reg->getHivePtr(hive);
        struct nk_key* k = cgl->reg->getKeyPtr(h, key_ofs);

        QList<CValue> vl = cgl->reg->listValues(h, k);

        if (!vl.isEmpty()) {
            beginRemoveRows(QModelIndex(),0,vl.count()-1);
            endRemoveRows();
        }
    }

    if (!key.isValid()) return;

    struct nk_key* ck;
    struct hive* h;
    if (!cgl->reg->keyPrepare(key.internalPointer(),h,hive,ck)) {
        hive = -1;
        key_ofs = -1;
        return;
    }
    key_ofs = cgl->reg->getKeyOfs(h,ck);

    QList<CValue> vl = cgl->reg->listValues(h, ck);
    if (!vl.isEmpty()) {
        beginInsertRows(QModelIndex(),0,vl.count()-1);
        endInsertRows();
    }

    if (table!=NULL) {
        table->resizeColumnsToContents();
        table->sortByColumn(0, Qt::AscendingOrder);
    }
}

void CValuesModel::renameValue(const QString &old_name, const QString &new_name)
{
    // TODO: rename
}

void CValuesModel::deleteValue(const QString &name)
{
    // TODO: delete
}

QString CValuesModel::getValueName(const QModelIndex &index)
{
    if (!index.isValid() || hive<0 || key_ofs<0)
        return QString();

    struct hive* h = cgl->reg->getHivePtr(hive);
    struct nk_key* k = cgl->reg->getKeyPtr(h, key_ofs);

    QList<CValue> vl = cgl->reg->listValues(h, k);

    int row = index.row();

    if  (row<0 || row>=vl.count()) return QString();
    CValue v = vl.at(row);

    if (v.name==QString("(Default)"))
        return QString();
    else
        return v.name;
}

int CValuesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (hive<0 || key_ofs<0)
        return 0;

    struct hive* h = cgl->reg->getHivePtr(hive);
    struct nk_key* k = cgl->reg->getKeyPtr(h, key_ofs);

    QList<CValue> vl = cgl->reg->listValues(h, k);

    return vl.count();
}

int CValuesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

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

    if  (row<0 || row>=vl.count()) return QVariant();
    CValue v = vl.at(row);

    if (role == Qt::DisplayRole) {
        if (col==0) {
            return v.name;
        } else if (col==1) {
            switch (v.type) {
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
        } else if (col==2) {
            if (v.type==REG_DWORD)
                return tr("0x%1 (%2)").arg(v.vDWORD,8,16,QChar('0')).arg(v.vDWORD);
            if (v.type==REG_SZ ||
                    v.type==REG_EXPAND_SZ ||
                    v.type==REG_MULTI_SZ)
                return v.vString;
            return QString::fromLatin1(vl.at(row).vOther.toHex());
        }
    } else if (role == Qt::DecorationRole) {
        if (col==0) {
            switch (v.type) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                case REG_MULTI_SZ:
                    return QIcon(":/icons/string");
                default:
                    return QIcon(":/icons/bin");
            }
        }
    }

    return QVariant();
}

Qt::ItemFlags CValuesModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)

    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant CValuesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation==Qt::Horizontal) {
        switch (section) {
            case 0: return QString("Name");
            case 1: return QString("Type");
            case 2: return QString("Value");
            default: return QVariant();
        }
    }
    return QVariant();
}


