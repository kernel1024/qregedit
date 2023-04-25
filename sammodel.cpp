#include "global.h"
#include "sammodel.h"

CSAMGroupsModel::CSAMGroupsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

CSAMGroupsModel::~CSAMGroupsModel() = default;

void CSAMGroupsModel::keyChanged(const QModelIndex &key, QTreeView *view)
{
    Q_UNUSED(view)

    // Close old key
    if (hive_num >= 0) {
        if (groups_count > 0) {
            beginRemoveRows(QModelIndex(), 0, groups_count - 1);
            endRemoveRows();
        }

        hive_num = -1;
        groups_count = 0;
    }

    // Exit if no valid key passed
    if (!key.isValid()) return;

    // Read new SAM
    struct nk_key *ck = nullptr;
    struct hive *h = nullptr;

    if (!cgl->reg->keyPrepare(key.internalPointer(), h, hive_num, ck)) {
        hive_num = -1;
        groups_count = 0;
        return;
    }

    if (h->type != HTYPE_SAM) {
        hive_num = -1;
        groups_count = 0;
        return;
    }

    groups_count = cgl->reg->listGroups(h).count();

    if (groups_count > 0) {
        beginInsertRows(QModelIndex(), 0, groups_count - 1);
        endInsertRows();
    }
}

inline quintptr gid2id(quint16 gid, qint16 member_idx)
{
    return (member_idx << 16) + gid;
}

inline void id2gid(quintptr id, quint16 &gid, qint16 &member_idx)
{
    gid = id & 0x0ffff;
    member_idx = (id >> 16) & 0x0ffff;
}

QModelIndex CSAMGroupsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent) || hive_num < 0)
        return QModelIndex();

    struct hive *h = cgl->reg->getHivePtr(hive_num);
    const QList<CGroup> grps = cgl->reg->listGroups(h);

    if (!parent.isValid()) { // top-level - group names
        if (row < 0 || row >= grps.count())
            return QModelIndex();

        return createIndex(row, column, gid2id(grps.at(row).grpid, -1));
    }

    qint16 mid = 0;
    quint16 gid = 0;
    id2gid(parent.internalId(), gid, mid);
    const int idx = grps.indexOf(CGroup(gid));

    if (idx >= 0) {
        if (row >= 0 && row < grps.at(idx).members.count())
            return createIndex(row, column, gid2id(gid, row));
    }

    return QModelIndex();
}

QModelIndex CSAMGroupsModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || hive_num < 0)
        return QModelIndex();

    struct hive *h = cgl->reg->getHivePtr(hive_num);
    const QList<CGroup> grps = cgl->reg->listGroups(h);

    qint16 mid = 0;
    quint16 gid = 0;
    id2gid(child.internalId(), gid, mid);

    if (mid < 0) // child is top-level item, group
        return QModelIndex();

    // child is username, return index of group and group's position
    const int idx = grps.indexOf(CGroup(gid));

    if (idx >= 0)
        return createIndex(idx, 0, gid2id(gid, -1));

    return QModelIndex();
}

int CSAMGroupsModel::rowCount(const QModelIndex &parent) const
{
    if (hive_num < 0 || parent.column() > 0)
        return 0;

    struct hive *h = cgl->reg->getHivePtr(hive_num);
    const QList<CGroup> grps = cgl->reg->listGroups(h);

    if (!parent.isValid())
        return grps.count();

    qint16 mid = 0;
    quint16 gid = 0;
    id2gid(parent.internalId(), gid, mid);

    const int idx = grps.indexOf(CGroup(gid));

    if (idx >= 0 && mid < 0)
        return grps.at(idx).members.count();

    return 0;
}

int CSAMGroupsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

QVariant CSAMGroupsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || hive_num < 0)
        return QVariant();

    struct hive *h = cgl->reg->getHivePtr(hive_num);
    const QList<CGroup> grps = cgl->reg->listGroups(h);

    qint16 mid = 0;
    quint16 gid = 0;
    id2gid(index.internalId(), gid, mid);
    const int idx = grps.indexOf(CGroup(gid));

    if (role == Qt::DisplayRole) {
        if (idx >= 0) {
            if (mid >= 0) { // username
                const CGroupMember m = grps.at(idx).members.at(mid);
                return QString("(0x%1) %2 (SID: %3)")
                    .arg(static_cast<quint16>(m.rid), 3, 16, QChar('0'))
                    .arg(m.name, m.sid);
            }

            // group
            return QString("(0x%1) %2")
                .arg(static_cast<quint16>(grps.at(idx).grpid), 3, 16, QChar('0'))
                .arg(grps.at(idx).name);
        }
    } else if (role == Qt::StatusTipRole) {
        if (idx >= 0 && mid < 0)
            return grps.at(idx).fullname;
    } else if (role == Qt::DecorationRole && idx >= 0) {
        if (mid >= 0)
            return QIcon::fromTheme(QSL("user-properties")); // TODO: move icon to local resources

        return QIcon::fromTheme(QSL("user-group-properties"));
    }

    return QVariant();
}

Qt::ItemFlags CSAMGroupsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

CSAMUsersModel::CSAMUsersModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

CSAMUsersModel::~CSAMUsersModel() = default;

void CSAMUsersModel::keyChanged(const QModelIndex &key, QTableView *view)
{
    // Close old key
    if (hive_num >= 0) {
        if (val_count > 0) {
            beginRemoveRows(QModelIndex(), 0, val_count - 1);
            endRemoveRows();
        }

        hive_num = -1;
        val_count = 0;
    }

    // Exit if no valid key passed
    if (!key.isValid()) return;

    // Read new SAM
    struct nk_key *ck = nullptr;
    struct hive *h = nullptr;

    if (!cgl->reg->keyPrepare(key.internalPointer(), h, hive_num, ck)) {
        hive_num = -1;
        val_count = 0;
        return;
    }

    if (h->type != HTYPE_SAM) {
        hive_num = -1;
        val_count = 0;
        return;
    }

    val_count = cgl->reg->listUsers(h).count();

    if (val_count > 0) {
        beginInsertRows(QModelIndex(), 0, val_count - 1);
        endInsertRows();
    }

    if (view != nullptr)
        view->resizeColumnsToContents();
}

int CSAMUsersModel::getUserRID(const QModelIndex &index) const
{
    if (!index.isValid() || hive_num < 0) return -1;

    struct hive *h = cgl->reg->getHivePtr(hive_num);
    const QList<CUser> ul = cgl->reg->listUsers(h);

    const int row = index.row();

    if  (row < 0 || row >= ul.count()) return -1;

    return ul.at(row).rid;
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
        return QSL("+");

    return QString();
}

QVariant CSAMUsersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || hive_num < 0)
        return QVariant();

    struct hive *h = cgl->reg->getHivePtr(hive_num);

    const QList<CUser> ul = cgl->reg->listUsers(h);

    const int row = index.row();
    const int col = index.column();

    if  (row < 0 || row >= ul.count()) return QVariant();

    const CUser &v = ul.at(row);

    if (role == Qt::DisplayRole) {
        switch (col) {
        case 0: // rid
            return QSL("0x%1").arg(static_cast<quint16>(v.rid), 0, 16, QChar('0'));

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
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("RID");

        case 1:
            return tr("Username");

        case 2:
            return tr("Admin?");

        case 3:
            return tr("Locked?");

        case 4:
            return tr("No password?");

        case 5:
            return tr("Full name");

        default:
            return QVariant();
        }
    }

    return QVariant();

}
