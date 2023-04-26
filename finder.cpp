#include <QApplication>
#include <QThread>
#include "finder.h"
#include "global.h"
#include <QDebug>

CFinder::CFinder(QObject *parent)
    : QObject(parent)
{
    hiveChanged(QModelIndex());
}

CFinder::~CFinder() = default;

void CFinder::searchText(const QModelIndex &idx, const QString &text)
{
    if (!idx.isValid() || text.isEmpty()) {
        hiveChanged(QModelIndex());
        Q_EMIT searchFinished();
        return;
    }

    struct nk_key* k = nullptr;
    struct hive* h = nullptr;
    int hive = 0;
    if (!cgl->reg->keyPrepare(idx.internalPointer(),h,hive,k)) {
        hiveChanged(QModelIndex());
        Q_EMIT searchFinished();
        return;
    }

    searchKeysOfsFlat = cgl->reg->listAllKeysOfsFlat(h,k);
    searchHive = hive;
    searchLastKeyIdx = -1;
    searchString = text;
    m_canceled = false;

    continueSearch();
}

void CFinder::continueSearch()
{
    if (searchKeysOfsFlat.isEmpty() || searchString.isEmpty() || m_canceled) return;

    struct hive *h = cgl->reg->getHivePtr(searchHive);
    if (h==nullptr) { // OOPS. No more hive. Clean all and exit.
        hiveChanged(QModelIndex());
        return;
    }

    Q_EMIT showProgressDialog();
    QThread::msleep(250);

    bool iok = false;
    const quint32 snum = searchString.toUInt(&iok);

    while (true) {
        searchLastKeyIdx++;
        if (searchLastKeyIdx>=searchKeysOfsFlat.count()) {
            searchLastKeyIdx=-1;
            Q_EMIT hideProgressDialog();
            Q_EMIT searchFinished();
            return;
        }

        struct nk_key *k = cgl->reg->getKeyPtr(h,searchKeysOfsFlat.at(searchLastKeyIdx));
        if (cgl->reg->getKeyName(h,k).contains(searchString,Qt::CaseInsensitive)) {
            Q_EMIT hideProgressDialog();
            Q_EMIT keyFound(reinterpret_cast<quintptr>(h),
                            reinterpret_cast<quintptr>(k), QString());
            return;
        }

        const QList<CValue> vl = cgl->reg->listValues(h, k);
        for(const auto &v : vl) {
            if (v.name.contains(searchString,Qt::CaseInsensitive) ||
                    v.vString.contains(searchString,Qt::CaseInsensitive) ||
                    v.vOther.contains(searchString.toUtf8()) ||
                    ((v.type==REG_DWORD)&&iok&&(v.vDWORD==snum)))
            {
                Q_EMIT hideProgressDialog();
                Q_EMIT keyFound(reinterpret_cast<quintptr>(h),
                                reinterpret_cast<quintptr>(k), v.name);
                return;
            }
        }

        QApplication::processEvents();
        if (m_canceled)
            break;
    }
    Q_EMIT hideProgressDialog();
}

void CFinder::destroyFinder()
{
    m_canceled = true;
    Q_EMIT requestToDestroy();
}

void CFinder::hiveChanged(const QModelIndex &idx)
{
    if (idx.isValid()) {
        struct nk_key* ck = nullptr;
        struct hive* h = nullptr;
        int hive = 0;
        if (cgl->reg->keyPrepare(idx.internalPointer(),h,hive,ck))
            if (searchHive!=hive) return;
    }
    searchHive = -1;
    searchKeysOfsFlat.clear();
    searchLastKeyIdx = -1;
    searchString.clear();
}
