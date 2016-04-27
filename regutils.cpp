#include "registrymodel.h"
#include "regutils.h"
#include "global.h"
#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QDateTime>
#include <QDebug>

CRegController::CRegController(QObject *parent)
    : QObject(parent)
{
    hives.clear();
}

QByteArray toUtf16(const QString &str)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    QTextEncoder *encoderWithoutBom = codec->makeEncoder( QTextCodec::IgnoreHeader );
    QByteArray ba  = encoderWithoutBom ->fromUnicode( str );
    ba.append('\0'); ba.append('\0');
    return ba;
}

QString fromUtf16(const QByteArray &str)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    QTextDecoder *decoderWithoutBom = codec->makeDecoder( QTextCodec::IgnoreHeader );
    QString s  = decoderWithoutBom->toUnicode( str );
    return s;
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
        int ret = QMessageBox::warning(0,tr("Registry Editor - Save hive"),
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
        int ret = QMessageBox::warning(0,tr("Registry Editor - Save hive"),
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
        QMessageBox::warning(0,tr("Registry Editor - Error"),
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

int CRegController::findKeyOfs(struct hive *hdesc, struct nk_key *key, const QString& name)
{
    int nkofs;
    struct ex_data ex;
    int count = 0, countri = 0;

    if (name.isEmpty()) return -3;

    nkofs = cgl->reg->getKeyOfs(hdesc, key);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs,0,16);
        return -1;
    }

    if (key->no_subkeys) {
        while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
            if (QString::compare(name,ex.name,Qt::CaseInsensitive)==0) {
                FREE(ex.name);
                return ex.nkoffs+4;
            }
            FREE(ex.name);
        }
    }

    return -2;
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

QString CRegController::getHivePrefix(struct hive* hdesc)
{
    QString ret;
    switch (hdesc->type) {
        case HTYPE_SAM: ret = QString("HKEY_LOCAL_MACHINE\\SAM"); break;
        case HTYPE_SYSTEM: ret = QString("HKEY_LOCAL_MACHINE\\SYSTEM"); break;
        case HTYPE_SECURITY: ret = QString("HKEY_LOCAL_MACHINE\\SECURITY"); break;
        case HTYPE_SOFTWARE: ret = QString("HKEY_LOCAL_MACHINE\\SOFTWARE"); break;
        case HTYPE_USER: ret = QString("HKEY_CURRENT_USER"); break;
        default: break;
    }
    return ret;
}

QString CRegController::getKeyName(struct hive* hdesc, struct nk_key* key)
{
    QString ret;
    if (getKeyOfs(hdesc,key)==(hdesc->rootofs+4))
        ret = getHivePrefix(hdesc);

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
            if (type==REG_MULTI_SZ)
                for (i = 0; i < (outlen-1); i++)
                    if (string[i] == 0) string[i] = '\n';
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

CValue CRegController::getValue(hive *hdesc, nk_key *key, const QString &name, int checkType)
{
    QList<CValue> vl = listValues(hdesc, key);
    int idx = -1;

    if (checkType>REG_NONE)
        idx = vl.indexOf(CValue(name, checkType));
    else {
        for (int i=0;i<vl.count();i++)
            if (vl.at(i).name==name) {
                idx = i;
                break;
            }
    }

    if (idx<0) {
        qWarning() << tr("Could not find %1 value in %2 key").arg(name,getKeyName(hdesc, key));
        return CValue();
    }
    return vl.at(idx);
}


QString CRegController::getKeyFullPath(struct hive *hdesc, struct nk_key *key, bool skipRoot)
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
    if (skipRoot)
        keys.removeFirst();
    return QString("\\"+keys.join("\\"));
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

QString escapeString(const QString& str)
{
    QString s = str;
    s.replace('\\',"\\\\");
    s.replace('\"',"\\\"");
    return s;
}

void exportBin(const QByteArray& data, int type, int &col, QTextStream &file)
{
    if (type == REG_BINARY) {
        file << "hex:";
        col += 4;
    } else {
        file << QString("hex(%1):").arg(type,0,16);
        col += 7;
    }
    int start = (col  - 2) / 3;
    for (int i=0;i<data.size();i++) {
        if (i<(data.size()-1))
            file << QString("%1,").arg((quint8)data.at(i),2,16,QChar('0'));
        else
            file << QString("%1").arg((quint8)data.at(i),2,16,QChar('0'));
        if (!((i + start) % 25)) file << "\\\r\n  ";
    }
    file << "\r\n";
}

bool CRegController::exportKey(struct hive *hdesc, struct nk_key *key, const QString &prefix, QTextStream &file)
{
    QString path = getKeyFullPath(hdesc, key, true);
    // export the key
    file << "\r\n";
    file << QString("[%1\%2]\r\n").arg(prefix, path);
    // export values
    QList<CValue> vl = listValues(hdesc, key);
    foreach (const CValue v, vl) {
        int col = 0;
        QString name = escapeString(v.name);

        /* print name */
        if (name.isEmpty()) {
            file << "@=";
            name = QString("@");
            col += 2;
        } else {
            file << QString("\"%1\"=").arg(name);
            col += name.length() + 3;
        }

        if(v.type == REG_DWORD)
        {
            file << QString("dword:%1\r\n").arg(v.vDWORD,8,16,QChar('0'));
        }
        else if(v.type == REG_SZ)
        {
            bool hex = false;
            for (int i=0;i<v.vString.length();i++)
                if (!v.vString.at(i).isPrint()) {
                    exportBin(v.vString.toUtf8(),v.type,col,file);
                    hex = true;
                    break;
                }
            if (!hex)
                file << QString("\"%1\"\r\n").arg(escapeString(v.vString));
        }
        else
        {
            exportBin(v.vOther,v.type,col,file);
        }
    }

    // export subkeys
    QList<int> kl = listKeysOfs(hdesc, key);
    foreach (const int ofs, kl)
        exportKey(hdesc, getKeyPtr(hdesc, ofs), prefix, file);

    return true;
}

struct nk_key* CRegController::navigateKey(struct hive *hdesc, const QString &path, bool allowCreate)
{
    struct nk_key* key = getKeyPtr(hdesc,hdesc->rootofs+4);

    QStringList kl = path.split('\\',QString::SkipEmptyParts);

    if (kl.isEmpty())
        key = NULL;

    while (!kl.isEmpty()) {
        QString kn = kl.takeFirst();
        int ofs = findKeyOfs(hdesc, key, kn);
        if (ofs<0) {
            if (allowCreate) {
                if (!createKey(hdesc,key,kn)) {
                    qCritical() << "navigateKey: failed to create key " << kn ;
                    return NULL;
                }
                ofs = findKeyOfs(hdesc, key, kn);
            } else {
                qCritical() << "navigateKey: child not found " << kn ;
                return NULL;
            }
        }
        key = getKeyPtr(hdesc, ofs);
    }

    return key;
}

QString CRegController::getOSInfo(struct hive *hdesc)
{
    struct nk_key *key = navigateKey(hdesc,"\\Microsoft\\Windows NT\\CurrentVersion");
    if (key==NULL) {
        qWarning() << "\\Microsoft\\Windows NT\\CurrentVersion key not found. This is not SOFTWARE hive?";
        return QString("\\Microsoft\\Windows NT\\CurrentVersion key not found. This is not SOFTWARE hive?");
    }
    CValue csd = getValue(hdesc, key, "CSDVersion", REG_SZ);
    CValue ver = getValue(hdesc, key, "CurrentVersion", REG_SZ);
    CValue prod = getValue(hdesc, key, "ProductName", REG_SZ);
    CValue date = getValue(hdesc, key, "InstallDate", REG_DWORD);
    CValue id = getValue(hdesc, key, "ProductId", REG_SZ);
    CValue build = getValue(hdesc, key, "CurrentBuildNumber", REG_SZ);

    QDateTime idate = QDateTime::fromTime_t(date.vDWORD);

    QString dpids;
    CValue dpid = getValue(hdesc, key, "DigitalProductId", REG_BINARY);
    if (dpid.isEmpty() || dpid.vOther.size()<67) {
        qWarning() << "DigitalProductID value is too short for decoding";
        dpids = tr("(failed to decode)");
    } else {
        const char digits[] = {'B','C','D','F','G','H','J','K',
                               'M','P','Q','R','T','V','W','X',
                               'Y','2','3','4','6','7','8','9'};

        const int result_len = 24;
        const int start_offset = 52;
        const int buf_len = 15;
        QByteArray buf = dpid.vOther.mid(start_offset,buf_len);

        for (int i = result_len; i >= 0; i--) {
            unsigned int x = 0;

            for (int j = buf_len - 1; j >= 0; j--) {
                x = (x << 8) + (quint8)buf.at(j);
                buf[j] = (quint8)(x / 24);
                x = x % 24;
            }
            dpids.prepend(QChar(digits[x]));
            if ((i%5==0) && (i>0))
                dpids.prepend(QChar('-'));
        }
    }

    return QString("%1 %2 (Version %3.%4)\n\n"
                   "Install date: %5\n\n"
                   "Product ID: %6\n"
                   "Digital Product ID: %7")
            .arg(prod.vString, csd.vString, ver.vString, build.vString)
            .arg(idate.toString(Qt::DefaultLocaleLongDate))
            .arg(id.vString, dpids);
}

QString unquoteString(const QString& s)
{
    QString a = s;
    if (!s.startsWith('"')) return a;
    a.remove(0,1);
    a.remove(a.length()-1,1);
    return a;
}

QString unescapeString(const QString& s)
{
    QString a = s;
    a.replace("\\\"","\"");
    a.replace("\\\\","\\");
    return a;
}

CValue parseValueStr(const QString& s)
{
    QString name;
    QString val;
    CValue v;

    // search for '=' outside quotes
    bool q = false;
    for (int i=0;i<s.length();i++) {
        if (s.at(i)==QChar('"')) q ^= 1;
        if ((s.at(i)==QChar('=')) && !q) {
            name = unescapeString(unquoteString(s.left(i)));
            val = unescapeString(unquoteString(s.mid(i+1)));
            break;
        }
    }
    if (name.isEmpty()) {
        qCritical() << "parseValueStr: empty value name parsed";
        return CValue();
    }

    if (val.startsWith("dword")) {
        v = CValue(REG_DWORD);
        val.remove(0,val.indexOf(':')+1);
        bool ok;
        v.vDWORD = val.toUInt(&ok,16);
        if (!ok) {
            qCritical() << "parseValueStr: incorrect hex dword value for " << v.name;
            return CValue();
        }
    } else if (val.startsWith("hex")) {
        int type = REG_BINARY;
        QString vt = val.mid(3,val.indexOf(':')-3);
        if (!vt.isEmpty()) {
            vt.remove(0,1); vt.remove(vt.length()-1,1);
            bool ok;
            type = vt.toInt(&ok,16);
            if (!ok) {
                qCritical() << "parseValueStr: failed to parse value type " << v.name;
                return CValue();
            }
        }
        v = CValue(type);
        val.remove(0,val.indexOf(':')+1);
        while (!val.isEmpty()) {
            bool ok;
            v.vOther.append((quint8)val.left(2).toInt(&ok,16));
            if (!ok) {
                qCritical() << "parseValueStr: failed to parse hex value " << v.name;
                return CValue();
            }
            val.remove(0,2);
            if (val.startsWith(','))
                val.remove(0,1);

        }
        if (v.type==REG_EXPAND_SZ ||
                v.type==REG_MULTI_SZ) {
            if (v.vOther.endsWith(QByteArray(2,0))) // remove trailing \0
                v.vOther.remove(v.vOther.size()-2,2);
            v.vString = fromUtf16(v.vOther);
            if (v.type==REG_MULTI_SZ)
                v.vString.replace(QChar(0),QChar('\n'));
        }
    } else {
        v = CValue(REG_SZ);
        v.vString = val;
    }
    v.name = name;
    return v;
}

bool CRegController::importReg(struct hive *hdesc, const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qCritical() << "importReg: failed to open file" << filename;
        return false;
    }
    QTextStream fs(&f);
    QString prefix = getHivePrefix(hdesc);

    QString sign = fs.readLine();
    if (!sign.startsWith("Windows Registry Editor Version 5.00")) {
        qCritical() << "importReg: file signature missing";
        return false;
    }

    struct nk_key *key = NULL;
    QString valacc;

    while (!fs.atEnd()) {
        QString s = fs.readLine().trimmed();

        if (s.startsWith('[')) { // keyname
            s.remove(0,1);
            s.remove(s.length()-1,1);
            if (!s.startsWith(prefix,Qt::CaseInsensitive)) {
                qCritical() << tr("importReg: could not import key %1 to %2 registry hive").arg(s,prefix);
                return false;
            }
            s.remove(prefix,Qt::CaseInsensitive);
            key = navigateKey(hdesc, s, true);
            if (key == NULL) {
                qCritical() << "importReg: failed to navigate key " << s;
                return false;
            }
            valacc.clear();
        } else if (!s.isEmpty()) {
            if (s.endsWith('\\')) { // string with wrap marker
                s.remove(s.length()-1,1);
                valacc+=s;
                continue;
            }
            s = valacc + s; // Now we have complete value string

            CValue v = parseValueStr(s);
            if (v.isEmpty()) {
                qCritical() << "importReg: failed to parse value " << s;
                return false;
            }

            QList<CValue> vl = listValues(hdesc, key);
            if (!vl.contains(v)) {
                if (!createValue(hdesc, key, v.type, v.name)) {
                    qCritical() << "importReg: failed to create value " << v.name;
                    return false;
                }
            }
            if (!setValue(hdesc, key, v)) {
                qCritical() << "importReg: failed to set value " << v.name;
                return false;
            }
            valacc.clear();
        }
    }
    f.close();

    return true;

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
                               value.name.toUtf8().data(),value.type,TPF_VK_EXACT)>=0;
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
    vDWORD = 0;
    vString.clear();
    vOther.clear();
}

CValue::CValue(int atype)
{
    name.clear();
    type = atype;
    vDWORD = 0;
    vString.clear();
    vOther.clear();
}

CValue::CValue(const QString &aname, int atype)
{
    name = aname;
    type = atype;
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
    vDWORD = vex.val;
    vString = str;
    vOther = data;
}

CValue &CValue::operator=(const CValue &other)
{
    name = other.name;
    type = other.type;
    vDWORD = other.vDWORD;
    vString = other.vString;
    vOther = other.vOther;

    return *this;
}

bool CValue::operator==(const CValue &ref) const
{
    return ((ref.name==name) && (ref.type==type));
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
