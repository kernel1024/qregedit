#include "global.h"
#include "sammodel.h"

CSAMGroupsModel::CSAMGroupsModel()
    : QAbstractItemModel()
{

}

void CSAMGroupsModel::keyChanged(const QModelIndex &key, QTreeView *view)
{

}

QModelIndex CSAMGroupsModel::index(int row, int column, const QModelIndex &parent) const
{
    return QModelIndex();

}

QModelIndex CSAMGroupsModel::parent(const QModelIndex &child) const
{
    return QModelIndex();

}

int CSAMGroupsModel::rowCount(const QModelIndex &parent) const
{
    return 0;
}

int CSAMGroupsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

QVariant CSAMGroupsModel::data(const QModelIndex &index, int role) const
{
    return QVariant();

}

Qt::ItemFlags CSAMGroupsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

CSAMUsersModel::CSAMUsersModel()
    : QAbstractTableModel()
{

}

void CSAMUsersModel::keyChanged(const QModelIndex &key, QTableView *view)
{
    // Close old key
    if (hive_num>=0) {
        if (val_count>0) {
            beginRemoveRows(QModelIndex(),0,val_count-1);
            endRemoveRows();
        }

        hive_num = -1;
        val_count = 0;
    }

    // Exit if no valid key passed
    if (!key.isValid()) return;

    // Read new SAM
    struct nk_key* ck;
    struct hive* h;
    if (!cgl->reg->keyPrepare(key.internalPointer(),h,hive_num,ck)) {
        hive_num = -1;
        val_count = 0;
        return;
    }
    if (h->type!=HTYPE_SAM) {
        hive_num = -1;
        val_count = 0;
        return;
    }

    val_count = cgl->reg->listUsers(h).count();

    if (val_count>0) {
        beginInsertRows(QModelIndex(),0,val_count-1);
        endInsertRows();
    }

    if (view!=NULL)
        view->resizeColumnsToContents();
}

int CSAMUsersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return val_count;
}

int CSAMUsersModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 6;
}

QString bool2string(bool val)
{
    if (val)
        return QString("+");
    else
        return QString();
}

QVariant CSAMUsersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || hive_num<0)
        return QVariant();

    struct hive* h = cgl->reg->getHivePtr(hive_num);

    QList<CUser> ul = cgl->reg->listUsers(h);

    int row = index.row();
    int col = index.column();

    if  (row<0 || row>=ul.count()) return QVariant();
    CUser v = ul.at(row);

    if (role == Qt::DisplayRole) {
        switch (col) {
            case 0: // rid
                return QString("0x%1").arg((quint16)v.rid,0,16,QChar('0'));
            case 1: // username
                return v.username;
            case 2: // admin?
                return bool2string(v.is_admin);
            case 3: // locked?
                return bool2string(v.is_locked);
            case 4: // no password?
                return bool2string(v.is_blank_pw);
            case 5: // full name
                return v.fullname;
            default:
                break;
        }
    } else if (role == Qt::ToolTipRole) {
        return QString("Comment: %1\n"
                       "Home path: %2\n"
                       "Profile path: %3\n"
                       "Drive letter: %4\n"
                       "Logon script: %5")
                .arg(v.comment,v.homeDir,v.profilePath,v.driveLetter,v.logonScript);
    }

    return QVariant();
}

Qt::ItemFlags CSAMUsersModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)

    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QVariant CSAMUsersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation==Qt::Horizontal) {
        switch (section) {
            case 0: return QString("RID");
            case 1: return QString("Username");
            case 2: return QString("Admin?");
            case 3: return QString("Locked?");
            case 4: return QString("No password?");
            case 5: return QString("Full name");
            default: return QVariant();
        }
    }
    return QVariant();

}
