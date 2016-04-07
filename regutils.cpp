#include "registrymodel.h"
#include "regutils.h"
#include "global.h"
#include <QDebug>

CRegController::CRegController(QObject *parent)
    : QObject(parent)
{
    hives.clear();
}

bool CRegController::openTopHive(const QString &filename)
{
    struct hive* h = openHive(filename.toUtf8().data(), cgl->hiveMode | HMODE_RO);
    if (h==NULL) {
        qCritical() << "Failed to open hive" << filename;
        return false;
    }
    if (h->type==HTYPE_UNKNOWN) {
        qCritical() << "Unable to detect hive type" << filename << " type " << h->type;
        closeHive(h);
        return false;
    }
    if (treeModel)
        treeModel->beginInsertRows(QModelIndex(),getHivesCount(),getHivesCount()+1);
    hives << h;
    if (treeModel)
        treeModel->endInsertRows();

    return true;
}


QStringList CRegController::listKeys(struct hive *hdesc, struct nk_key *key)
{
    int nkofs;
    struct ex_data ex;
    int count = 0, countri = 0;

    QStringList keys;

    nkofs = (quintptr)key - (quintptr)(hdesc->buffer);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs,0,16);
        return keys;
    }

    if (key->no_subkeys) {
        while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
            keys << QString(ex.name);
            FREE(ex.name);
        }
    }

    return keys;
}

QList<int> CRegController::listKeysOfs(struct hive *hdesc, struct nk_key *key)
{
    int nkofs;
    struct ex_data ex;
    int count = 0, countri = 0;

    QList<int> keys;

    nkofs = (quintptr)key - (quintptr)(hdesc->buffer);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs,0,16);
        return keys;
    }

    if (key->no_subkeys) {
        while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
            keys << ex.nkoffs+4;
            FREE(ex.name);
        }
    }

    return keys;
}

struct nk_key * CRegController::getKeyPtr(struct hive* hdesc, int nkofs)
{
    return (struct nk_key *)(hdesc->buffer + nkofs);
}

int CRegController::getKeyOfs(struct hive* hdesc, struct nk_key* key)
{
    return (int)((quintptr)key - (quintptr)(hdesc->buffer));
}

QString CRegController::getKeyName(struct hive* hdesc, struct nk_key* key)
{
    QString ret;
    if (getKeyOfs(hdesc,key)==(hdesc->rootofs+4)) {
        switch (hdesc->type) {
            case HTYPE_SAM: ret = QString("HKEY_SAM"); break;
            case HTYPE_SYSTEM: ret = QString("HKEY_SYSTEM"); break;
            case HTYPE_SECURITY: ret = QString("HKEY_SECURITY"); break;
            case HTYPE_SOFTWARE: ret = QString("HKEY_SOFTWARE"); break;
            case HTYPE_USER: ret = QString("HKEY_USER"); break;
            default: break;
        }
    }
    if (!ret.isEmpty())
        return ret;

    if (key->len_name <= 0) {
        printf("ex_next: nk at 0x%0x has no name!\n",(quintptr)key);
    } else if (key->type & 0x20) {
        ret = QString::fromLocal8Bit(key->keyname,key->len_name);
    } else {
        char *name = string_regw2prog(key->keyname, key->len_name);
        ret = QString(name);
        FREE(name);
    }
    return ret;
}

QString CRegController::getKeyTooltip(struct hive *hdesc, struct nk_key *key)
{
    QString ret;
    if (getKeyOfs(hdesc,key)==(hdesc->rootofs+4))
        ret = QString::fromLocal8Bit(hdesc->filename);
    return ret;
}

QString CRegController::getKeyFullPath(struct hive *hdesc, struct nk_key *key)
{
    QStringList keys;
    struct nk_key* k = key;
    keys.prepend(getKeyName(hdesc, k));
    while (getKeyOfs(hdesc,k)!=(hdesc->rootofs+4)) {
        k = getKeyPtr(hdesc, k->ofs_parent + 0x1004);
        if (!checkKey(k))
            return QString();
        keys.prepend(getKeyName(hdesc, k));
    }
    return QString("\\\\"+keys.join("\\"));
}

int CRegController::getHive(const struct nk_key *key) const
{
    quintptr k = (quintptr)(key);
    for (int i=0;i<hives.count();i++) {
        quintptr hive = (quintptr)(hives.at(i)->buffer);
        quintptr size = hives.at(i)->size;
        if ((k>=hive) && (k<(hive+size)))
            return i;
    }
    return -1;
}

bool CRegController::checkKey(const nk_key *key) const
{
    if (getHive(key)==-1) {
        printf("Error: 'nk' not assigned to opened hives, at offset 0x%0x!\n",(quintptr)key);
        qFatal("fatal");
        return false;
    }
    if (key->id != 0x6b6e) {
        printf("Error: Not a 'nk' node at offset 0x%0x!\n",(quintptr)key);
        qFatal("fatal");
        return false;
    }
    return true;
}

bool CRegController::keyPrepare(const void *ptr, struct hive *&hive, int& hnum, struct nk_key *&key) const
{
    key = (struct nk_key*)ptr;
    if (!checkKey(key))
        return false;

    hnum = getHive(key);
    if (hnum==-1)
        return false;

    hive = hives.at(hnum);
    return true;
}
