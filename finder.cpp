#include <QApplication>
#include <QThread>
#include "finder.h"
#include "global.h"
#include <QDebug>

CFinder::CFinder(QObject *parent)
    : QObject(parent)
{
    hiveChanged(QModelIndex());
    m_canceled = false;
}

void CFinder::searchText(const QModelIndex &idx, const QString &text)
{
    if (!idx.isValid() || text.isEmpty()) {
        hiveChanged(QModelIndex());
        emit searchFinished();
        return;
    }

    struct nk_key* k;
    struct hive* h;
    int hive;
    if (!cgl->reg->keyPrepare(idx.internalPointer(),h,hive,k)) {
        hiveChanged(QModelIndex());
        emit searchFinished();
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
    if (h==NULL) { // OOPS. No more hive. Clean all and exit.
        hiveChanged(QModelIndex());
        return;
    }

    emit showProgressDialog();
    QThread::msleep(250);

    bool iok;
    quint32 snum = searchString.toInt(&iok);

    while (true) {
        searchLastKeyIdx++;
        if (searchLastKeyIdx>=searchKeysOfsFlat.count()) {
            searchLastKeyIdx=-1;
            emit hideProgressDialog();
            emit searchFinished();
            return;
        }

        struct nk_key *k = cgl->reg->getKeyPtr(h,searchKeysOfsFlat.at(searchLastKeyIdx));
        if (cgl->reg->getKeyName(h,k).contains(searchString,Qt::CaseInsensitive)) {
            emit hideProgressDialog();
            emit keyFound(h, k,QString());
            return;
        }

        QList<CValue> vl = cgl->reg->listValues(h, k);
        foreach (const CValue v, vl)
            if (v.name.contains(searchString,Qt::CaseInsensitive) ||
                    v.vString.contains(searchString,Qt::CaseInsensitive) ||
                    v.vOther.contains(searchString.toUtf8()) ||
                    ((v.type==REG_DWORD)&&iok&&(v.vDWORD==snum)))
            {
                emit hideProgressDialog();
                emit keyFound(h, k, v.name);
                return;
            }

        QApplication::processEvents();
        if (m_canceled)
            break;
    }
    emit hideProgressDialog();
}

void CFinder::hiveChanged(const QModelIndex &idx)
{
    if (idx.isValid()) {
        struct nk_key* ck;
        struct hive* h;
        int hive;
        if (cgl->reg->keyPrepare(idx.internalPointer(),h,hive,ck))
            if (searchHive!=hive) return;
    }
    searchHive = -1;
    searchKeysOfsFlat.clear();
    searchLastKeyIdx = -1;
    searchString.clear();
}
