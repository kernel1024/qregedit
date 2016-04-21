#include "registrymodel.h"
#include "regutils.h"
#include "global.h"
#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>

CRegController::CRegController(QObject *parent)
    : QObject(parent)
{
    hives.clear();
}

QByteArray CRegController::toUtf16(const QString &str)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    QTextEncoder *encoderWithoutBom = codec->makeEncoder( QTextCodec::IgnoreHeader );
    QByteArray ba  = encoderWithoutBom ->fromUnicode( str );
    ba.append('\0'); ba.append('\0');
    return ba;
}

bool CRegController::openTopHive(const QString &filename, int mode)
{
    struct hive* h = openHive(filename.toUtf8().data(), cgl->hiveOpenMode | mode);
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
        treeModel->beginInsertRows(QModelIndex(),getHivesCount(),getHivesCount());
    hives << h;
    if (treeModel)
        treeModel->endInsertRows();

    emit hiveOpened(getHivesCount()-1);

    return true;
}

bool CRegController::saveTopHive(int idx)
{
    if (idx<0 || idx>=hives.count()) return false;

    struct hive* h = hives.at(idx);

    if ((h->state & HMODE_DIRTY) == 0)
        return true;
    else {
        int ret = QMessageBox::warning(0,tr("Registry Editor"),
                                       tr("Hive file '%1' has been modified. "
                                          "Do you want to save hive?").arg(h->filename),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                       QMessageBox::Save);
        if (ret==QMessageBox::Discard)
            return true;
        if (ret==QMessageBox::Cancel)
            return false;
    }

    if (h->state & HMODE_DIDEXPAND) {
        int ret = QMessageBox::warning(0,tr("Registry Editor"),
                                       tr("Hive file '%1' has been expanded. "
                                          "This is highly experimental feature!\n"
                                          "Please, use backups and use this feature at own risk!\n\n"
                                          "Do you want to save hive anyway?").arg(h->filename),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                       QMessageBox::Save);
        if (ret==QMessageBox::Discard)
            return true;
        if (ret==QMessageBox::Cancel)
            return false;
    }

    if (!writeHive(h)) {
        emit hiveSaved(idx);
        return true;
    } else {
        QMessageBox::warning(0,tr("Registry Editor"),
                             tr("Failed to save hive file '%1'.").arg(h->filename));
        return false;
    }
}

void CRegController::closeTopHive(int idx)
{
    if (idx<0 || idx>=hives.count()) return;

    emit hiveAboutToClose(idx);

    struct hive* h = hives.at(idx);

    if (treeModel)
        treeModel->beginRemoveRows(QModelIndex(),idx,idx);
    hives.removeAt(idx);
    if (treeModel)
        treeModel->endRemoveRows();

    QApplication::processEvents();
    closeHive(h);

    emit hiveClosed(idx);
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

QList<CValue> CRegController::listValues(struct hive *hdesc, struct nk_key *key)
{
    int nkofs;
    int count = 0;
    struct vex_data vex;

    QList<CValue> vals;

    nkofs = (quintptr)key - (quintptr)(hdesc->buffer);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs,0,16);
        return vals;
    }

    if (key->no_values) {
        while ((ex_next_v(hdesc, nkofs, &count, &vex) > 0)) {
            QString str;
            QVariant v = getValue(hdesc, vex, false);
            if (v.isNull()) {
                FREE(vex.name);
                continue;
            }
            if (strcmp(v.typeName(),"QString")==0)
                str = v.toString();

            vals << CValue(vex,str,getValue(hdesc, vex, true).toByteArray());
            FREE(vex.name);
        }
    }

    return vals;
}

QList<int> CRegController::listKeysOfs(struct hive *hdesc, struct nk_key *key)
{
    int nkofs;
    struct ex_data ex;
    int count = 0, countri = 0;

    QList<int> keys;

    nkofs = cgl->reg->getKeyOfs(hdesc, key);

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

QList<int> CRegController::listAllKeysOfsFlat(struct hive *hdesc, struct nk_key *key)
{
    QList<int> childs = listKeysOfs(hdesc, key);
    QList<int> accu;
    for (int i=0;i<childs.count();i++)
        accu.append(listAllKeysOfsFlat(hdesc,getKeyPtr(hdesc,childs.at(i))));
    childs.append(accu);
    return childs;
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
        qWarning() << tr("CRegController::getKeyName: nk at 0x%1 has no name!").arg((quintptr)key,8,16);
    } else if (key->type & 0x20) {
        ret = QString::fromLocal8Bit(key->keyname,key->len_name);
    } else {
        int outlen;
        char *name = string_regw2prog(key->keyname, key->len_name, &outlen);
        ret = QString(name);
        FREE(name);
    }
    return ret;
}

QString CRegController::getKeyTooltip(struct hive *hdesc, struct nk_key *key)
{
    QString ret;
    if (getKeyOfs(hdesc,key)==(hdesc->rootofs+4))
        ret = QString(hdesc->filename);
    return ret;
}

struct keyval *CRegController::getKeyValue(struct hive *hdesc, struct nk_key *key, keyval *kv,
                                           const QString& name, int type, int exact)
{
    int nkofs;
    int count = 0;
    struct vex_data vex;
    struct keyval *nkv = kv;

    nkofs = cgl->reg->getKeyOfs(hdesc, key);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs,0,16);
        return nkv;
    }

    if (key->no_values) {
        while ((ex_next_v(hdesc, nkofs, &count, &vex) > 0)) {
            if (QString(vex.name)==name) {
                nkv = getKeyValue(hdesc, nkv, vex, type, exact);
                FREE(vex.name);
                break;
            }
            FREE(vex.name);
        }
    }

    return nkv;
}

struct keyval *CRegController::getKeyValue(struct hive *hdesc, struct keyval *kv,
                           struct vex_data vex, int type, int exact )
{
    int l,i,parts,list,blockofs,blocksize,point,copylen,restlen;
    struct keyval *kr;
    void *keydataptr = NULL;
    struct db_key *db;
    void *addr;

    l = vex.size;
    if (l == -1) return(NULL);  /* error */
    if (kv && (kv->len < l)) return(NULL); /* Check for overflow of supplied buffer */

    if ((quint32)(vex.vk->len_data) == 0x80000000 && (exact & TPF_VK_SHORT)) {
        /* Special inline case (len = 0x80000000) */
        keydataptr = (&vex.vk->val_type); /* Data (4 bytes?) in type field */
    }
    if (type && vex.vk->val_type && (vex.vk->val_type) != type)
        keydataptr = NULL;
    else if (vex.vk->len_data & 0x80000000)
        keydataptr = (&vex.vk->ofs_data);
    else
        keydataptr = (hdesc->buffer + vex.vk->ofs_data + 0x1004);

    if (keydataptr==NULL)
        return NULL;

    /* Allocate space for data + header, or use supplied buffer */
    if (kv) {
        kr = kv;
    } else {
        kr = (keyval*)calloc(1,l*sizeof(int)+4);
    }

    kr->len = l;

    if (l > VAL_DIRECT_LIMIT) {       /* Where do the db indirects start? seems to be around 16k */
        db = (struct db_key *)keydataptr;
        if (db->id != 0x6264) {
            qCritical() << "CRegController::getKeyValue: invalid db_key structure found for value " << vex.name;
            return NULL;
        }
        parts = db->no_part;
        list = db->ofs_data + 0x1004;

        point = 0;
        restlen = l;
        for (i = 0; i < parts; i++) {
            blockofs = get_int(hdesc->buffer + list + (i << 2)) + 0x1000;
            blocksize = -get_int(hdesc->buffer + blockofs) - 8;

            /* Copy this part, up to size of block or rest lenght in last block */
            copylen = (blocksize > restlen) ? restlen : blocksize;

            addr = (void*)((quintptr)&(kr->data) + point);
            memcpy( addr, hdesc->buffer + blockofs + 4, copylen);

            point += copylen;
            restlen -= copylen;
        }
    } else {
        if (l && kr && keydataptr) memcpy(&(kr->data), keydataptr, l);
    }

    return(kr);
}

QVariant CRegController::getValue(struct hive *hdesc, struct vex_data vex, int forceHex)
{
    void *data;
    int len,i,type;
    char *string = NULL;
    struct keyval *kv = NULL;

    QVariant res;
    type = vex.type;
    len = vex.size;

    kv = getKeyValue(hdesc, NULL, vex, 0, TPF_VK);

    if (!kv) {
        qCritical() << "Value - could not fetch data" << vex.name;
        return QVariant();
    }

    data = (void *)&(kv->data);

    if (forceHex)
        type = REG_BINARY;

    switch (type) {
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
            int outlen;
            string = string_regw2prog(data, len, &outlen);
            for (i = 0; i < outlen; i++) {
                if (string[i] == 0) string[i] = '\n';
                if (type == REG_SZ) break;
            }
            res = QString(string);
            FREE(string);
            break;
        case REG_DWORD:
            res = QVariant::fromValue((quint32)*(unsigned short *)data);
            break;
        default:
            res = QByteArray((char *)data, len);
    }
    FREE(kv);

    return res;
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

bool CRegController::createKey(hive *hdesc, nk_key *parent, const QString &name)
{
    QString s = name;
    return (add_key(hdesc, cgl->reg->getKeyOfs(hdesc, parent), s.toUtf8().data())!=NULL);
}

void CRegController::deleteKey(hive *hdesc, nk_key *parent, const QString &name)
{
    QString s = name;
    rdel_keys(hdesc, s.toUtf8().data(), cgl->reg->getKeyOfs(hdesc, parent));
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
        qCritical() << tr("Error: 'nk' not assigned to opened hives, at offset 0x%1!\n").arg((quintptr)key);
        return false;
    }
    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%0x!\n").arg((quintptr)key);
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

QString CRegController::getValueTypeStr(int type)
{
    if (type>=REG_NONE && type<REG_MAX)
        return QString(val_types[type]);
    else
        return QString();
}

bool CRegController::setValue(struct hive *hdesc, struct nk_key* key, const CValue &value)
{
    struct keyval *newkv = NULL;
    int newsize = 0;
    QByteArray str;
    QString s = value.vString;

    switch(value.type) {
        case REG_DWORD:
            newkv = getKeyValue(hdesc, key, NULL, value.name, value.type, TPF_VK);
            newkv->data = value.vDWORD;
            break;
        case REG_MULTI_SZ:
            if (value.type==REG_MULTI_SZ)
                s.replace(QChar('\n'),QChar(0));
        case REG_SZ:
        case REG_EXPAND_SZ:
            str = toUtf16(s);
            newsize = str.size();
            newkv=(struct keyval*)calloc(1,newsize+sizeof(int));
            newkv->len = newsize;
            memcpy((char*)&(newkv->data),str.data(),newsize);
            break;
        default: // blob
            newsize = value.vOther.size();
            newkv = (struct keyval*)calloc(1,newsize+sizeof(int)+4);
            memcpy((char *)&(newkv->data),value.vOther.data(),value.vOther.size());
            newkv->len = newsize;
            break;
    }

    if (newkv!=NULL) {
        bool res = put_buf2val(hdesc,newkv,cgl->reg->getKeyOfs(hdesc,key),
                               value.name.toUtf8().data(),value.type,TPF_VK_EXACT)!=0;
        FREE(newkv);
        return res;
    }

    return false;

}

bool CRegController::deleteValue(struct hive *hdesc, struct nk_key *key, const QString &vname)
{
    QString s = vname;
    return del_value(hdesc, cgl->reg->getKeyOfs(hdesc, key), s.toUtf8().data(), TPF_EXACT)==0;
}

bool CRegController::createValue(struct hive *hdesc, struct nk_key *key, int vtype, const QString &vname)
{
    QString s = vname;
    return add_value(hdesc, cgl->reg->getKeyOfs(hdesc, key), s.toUtf8().data(), vtype)!=NULL;
}

CValue::CValue()
{
    name.clear();
    type = REG_NONE;
    size = 0;
    vDWORD = 0;
    vString.clear();
    vOther.clear();
}

CValue::CValue(int atype)
{
    name.clear();
    type = atype;
    size = 0;
    vDWORD = 0;
    vString.clear();
    vOther.clear();
}

CValue::CValue(struct vex_data vex, const QString& str, const QByteArray& data)
{
    name = QString(vex.name);
    if (name.isEmpty())
        name = QString("@");
    type = vex.type;
    size = vex.size;
    vDWORD = vex.val;
    vString = str;
    vOther = data;
}

CValue &CValue::operator=(const CValue &other)
{
    name = other.name;
    type = other.type;
    size = other.size;
    vDWORD = other.vDWORD;
    vString = other.vString;
    vOther = other.vOther;

    return *this;
}

bool CValue::operator==(const CValue &ref) const
{
    return ((ref.name==name) && (ref.type==type) && (ref.size==size));
}

bool CValue::operator!=(const CValue &ref) const
{
    return !operator ==(ref);
}

bool CValue::isEmpty() const
{
    return (name.isEmpty() && type==REG_NONE);
}

bool CValue::isDefault() const
{
    return ((name.isEmpty() || name==QString("@")) && !isEmpty());
}
