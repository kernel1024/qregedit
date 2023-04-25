#include "registrymodel.h"
#include "regutils.h"
#include "global.h"
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#if QT_VERSION >= 0x060000
#include <QStringEncoder>
#include <QStringDecoder>
#else
#include <QTextCodec>
#endif
#include <QDebug>

CRegController::CRegController(QObject *parent)
    : QObject(parent)
{
}

QByteArray toUtf16(const QString &str)
{
#if QT_VERSION >= 0x060000
    auto encoder = QStringEncoder(QStringEncoder::Utf16);
    QByteArray encodedString = encoder(str);
    return encodedString;
#else
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    QTextEncoder *encoderWithoutBom = codec->makeEncoder( QTextCodec::IgnoreHeader );
    QByteArray ba  = encoderWithoutBom ->fromUnicode( str );
    delete encoderWithoutBom;
    ba.append('\0');
    ba.append('\0');
    return ba;
#endif
}

QString fromUtf16(const QByteArray &str)
{
#if QT_VERSION >= 0x060000
    auto decoder = QStringDecoder(QStringDecoder::Utf16);
    QString string = decoder(str);
    return string;
#else
    QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    QTextDecoder *decoderWithoutBom = codec->makeDecoder( QTextCodec::IgnoreHeader );
    QString s  = decoderWithoutBom->toUnicode( str );
    delete decoderWithoutBom;
    return s;
#endif
}

bool CRegController::openTopHive(const QString &filename, int mode)
{
    struct hive *h = openHive(filename.toUtf8().data(), cgl->hiveOpenMode | mode);

    if (h == nullptr) {
        qCritical() << "Failed to open hive" << filename;
        return false;
    }

    if (h->type == HTYPE_UNKNOWN) {
        qCritical() << "Unable to detect hive type" << filename << " type " << h->type;
        closeHive(h);
        return false;
    }

    if (treeModel)
        treeModel->beginInsertRows(QModelIndex(), getHivesCount(), getHivesCount());

    hives << h;

    if (treeModel)
        treeModel->endInsertRows();

    Q_EMIT hiveOpened(getHivesCount() - 1);

    return true;
}

bool CRegController::saveTopHive(int idx)
{
    if (idx < 0 || idx >= hives.count()) return false;

    struct hive *h = hives.at(idx);

    if ((h->state & HMODE_DIRTY) == 0)
        return true;

    const int ret = QMessageBox::warning(nullptr,
                                         tr("Registry Editor - Save hive"),
                                         tr("Hive file '%1' has been modified. "
                                            "Do you want to save hive?")
                                             .arg(h->filename),
                                         QMessageBox::Save | QMessageBox::Discard
                                             | QMessageBox::Cancel,
                                         QMessageBox::Save);

    if (ret == QMessageBox::Discard)
        return true;

    if (ret == QMessageBox::Cancel)
        return false;

    if ((h->state & HMODE_DIDEXPAND) != 0) {
        const int ret
            = QMessageBox::warning(nullptr,
                                   tr("Registry Editor - Save hive"),
                                   tr("Hive file '%1' has been expanded. "
                                      "This is highly experimental feature!\n"
                                      "Please, use backups and use this feature at own risk!\n\n"
                                      "Do you want to save hive anyway?")
                                       .arg(h->filename),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                   QMessageBox::Save);

        if (ret == QMessageBox::Discard)
            return true;

        if (ret == QMessageBox::Cancel)
            return false;
    }

    if (writeHive(h) == 0) {
        Q_EMIT hiveSaved(idx);
        return true;
    }
    QMessageBox::warning(nullptr,
                         tr("Registry Editor - Error"),
                         tr("Failed to save hive file '%1'.").arg(h->filename));
    return false;
}

void CRegController::closeTopHive(int idx)
{
    if (idx < 0 || idx >= hives.count()) return;

    Q_EMIT hiveAboutToClose(idx);

    struct hive *h = hives.at(idx);

    if (treeModel)
        treeModel->beginRemoveRows(QModelIndex(), idx, idx);

    hives.removeAt(idx);

    if (treeModel)
        treeModel->endRemoveRows();

    QApplication::processEvents();
    closeHive(h);

    Q_EMIT hiveClosed(idx);
}

struct hive *CRegController::getHivePtr(int idx)
{
    if (idx >= 0 && idx < hives.count()) {
        return hives.at(idx);
    }
    return nullptr;
}

QStringList CRegController::listKeys(struct hive *hdesc, struct nk_key *key)
{
    int nkofs = 0;
    struct ex_data ex
    {};
    int count = 0;
    int countri = 0;

    QStringList keys;

    nkofs = (quintptr)key - (quintptr)(hdesc->buffer);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs, 0, 16);
        return keys;
    }

    if (key->no_subkeys != 0) {
        while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
            keys << QString(ex.name);
            FREE(ex.name);
        }
    }

    return keys;
}

QList<CValue> CRegController::listValues(struct hive *hdesc, struct nk_key *key, int exact)
{
    int nkofs = 0;
    int count = 0;
    struct vex_data vex
    {};

    QList<CValue> vals;

    nkofs = (quintptr)key - (quintptr)(hdesc->buffer);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs, 0, 16);
        return vals;
    }

    if (key->no_values != 0) {
        while ((ex_next_v(hdesc, nkofs, &count, &vex) > 0)) {
            QString str;
            const QVariant v = getValue(hdesc, vex, false, exact);

            if (v.isNull()) {
                FREE(vex.name);
                continue;
            }

            if (strcmp(v.typeName(), "QString") == 0)
                str = v.toString();

            vals << CValue(vex, str, getValue(hdesc, vex, true).toByteArray());
            FREE(vex.name);
        }
    }

    return vals;
}

int CRegController::findKeyOfs(struct hive *hdesc, struct nk_key *key, const QString &name)
{
    int nkofs = 0;
    struct ex_data ex {};
    int count = 0;
    int countri = 0;

    if (name.isEmpty()) return -3;

    nkofs = getKeyOfs(hdesc, key);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs, 0, 16);
        return -1;
    }

    if (key->no_subkeys != 0) {
        while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
            if (QString::compare(name, ex.name, Qt::CaseInsensitive) == 0) {
                FREE(ex.name);
                return ex.nkoffs + 4;
            }

            FREE(ex.name);
        }
    }

    return -2;
}

QList<int> CRegController::listKeysOfs(struct hive *hdesc, struct nk_key *key)
{
    int nkofs = 0;
    struct ex_data ex {};
    int count = 0;
    int countri = 0;

    QList<int> keys;

    nkofs = getKeyOfs(hdesc, key);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs, 0, 16);
        return keys;
    }

    if (key->no_subkeys != 0) {
        while ((ex_next_n(hdesc, nkofs, &count, &countri, &ex) > 0)) {
            keys << ex.nkoffs + 4;
            FREE(ex.name);
        }
    }

    return keys;
}

QList<int> CRegController::listAllKeysOfsFlat(struct hive *hdesc, struct nk_key *key)
{
    QList<int> childs = listKeysOfs(hdesc, key);
    QList<int> accu;

    for (int i = 0; i < childs.count(); i++)
        accu.append(listAllKeysOfsFlat(hdesc, getKeyPtr(hdesc, childs.at(i))));

    childs.append(accu);
    return childs;
}

struct nk_key *CRegController::getKeyPtr(struct hive *hdesc, int nkofs)
{
    return (struct nk_key *)(hdesc->buffer + nkofs);
}

int CRegController::getKeyOfs(struct hive *hdesc, struct nk_key *key)
{
    return (int)((quintptr)key - (quintptr)(hdesc->buffer));
}

QString CRegController::getHivePrefix(struct hive *hdesc)
{
    QString ret;

    switch (hdesc->type) {
        case HTYPE_SAM:
            ret = QSL("HKEY_LOCAL_MACHINE\\SAM");
            break;

        case HTYPE_SYSTEM:
            ret = QSL("HKEY_LOCAL_MACHINE\\SYSTEM");
            break;

        case HTYPE_SECURITY:
            ret = QSL("HKEY_LOCAL_MACHINE\\SECURITY");
            break;

        case HTYPE_SOFTWARE:
            ret = QSL("HKEY_LOCAL_MACHINE\\SOFTWARE");
            break;

        case HTYPE_USER:
            ret = QSL("HKEY_CURRENT_USER");
            break;

        default:
            break;
    }

    return ret;
}

QString CRegController::getKeyName(struct hive *hdesc, struct nk_key *key)
{
    QString ret;

    if (getKeyOfs(hdesc, key) == (hdesc->rootofs + 4))
        ret = getHivePrefix(hdesc);

    if (!ret.isEmpty())
        return ret;

    if (key->len_name <= 0) {
        qWarning() << tr("CRegController::getKeyName: nk at 0x%1 has no name!").arg((quintptr)key, 8, 16);
    } else if ((key->type & 0x20) != 0) {
        ret = QString::fromLocal8Bit(key->keyname, key->len_name);
    } else {
        int outlen = 0;
        char *name = string_regw2prog(key->keyname, key->len_name, &outlen);
        ret = QString(name);
        FREE(name);
    }

    return ret;
}

QString CRegController::getKeyTooltip(struct hive *hdesc, struct nk_key *key)
{
    QString ret;

    if (getKeyOfs(hdesc, key) == (hdesc->rootofs + 4))
        ret = QString(hdesc->filename);

    return ret;
}

struct keyval *CRegController::getKeyValue(struct hive *hdesc, struct nk_key *key, keyval *kv,
        const QString &name, int type, int exact)
{
    int nkofs = 0;
    int count = 0;
    struct vex_data vex
    {};
    struct keyval *nkv = kv;

    nkofs = getKeyOfs(hdesc, key);

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%1!").arg(nkofs, 0, 16);
        return nkv;
    }

    if (key->no_values != 0) {
        while ((ex_next_v(hdesc, nkofs, &count, &vex) > 0)) {
            if (QString(vex.name) == name) {
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
        const struct vex_data &vex, int type, int exact )
{
    void *keydataptr = nullptr;
    struct keyval *kr = nullptr;

    const int l = vex.size;

    if (l == -1) return (nullptr); /* error */

    if (kv && (kv->len < l)) return (nullptr); /* Check for overflow of supplied buffer */

    if ((quint32) (vex.vk->len_data) == 0x80000000 && ((exact & TPF_VK_SHORT) != 0)) {
        /* Special inline case (len = 0x80000000) */
        keydataptr = (&vex.vk->val_type); /* Data (4 bytes?) in type field */
    }

    if ((type != 0) && (vex.vk->val_type != 0) && (vex.vk->val_type) != type) {
        keydataptr = nullptr;
    } else if ((vex.vk->len_data & 0x80000000) != 0U) {
        keydataptr = (&vex.vk->ofs_data);
    } else {
        keydataptr = (hdesc->buffer + vex.vk->ofs_data + 0x1004);
    }

    if (keydataptr == nullptr)
        return nullptr;

    /* Allocate space for data + header, or use supplied buffer */
    if (kv) {
        kr = kv;
    } else {
        kr = (keyval *)calloc(1, l * sizeof(int) +4);
    }

    if (!kr) return (nullptr);

    kr->len = l;

    if (l > VAL_DIRECT_LIMIT) {       /* Where do the db indirects start? seems to be around 16k */
        auto *db = (struct db_key *)keydataptr;

        if (db->id != 0x6264) {
            qCritical() << "CRegController::getKeyValue: invalid db_key structure found for value " << vex.name;
            return nullptr;
        }

        const int parts = db->no_part;
        const int list = db->ofs_data + 0x1004;

        int point = 0;
        int restlen = l;

        for (int i = 0; i < parts; i++) {
            const int blockofs = get_int(hdesc->buffer + list + (i << 2)) + 0x1000;
            const int blocksize = -get_int(hdesc->buffer + blockofs) - 8;

            /* Copy this part, up to size of block or rest lenght in last block */
            const int copylen = (blocksize > restlen) ? restlen : blocksize;

            auto *addr = (void *)((quintptr) & (kr->data) + point);
            memcpy( addr, hdesc->buffer + blockofs + 4, copylen);

            point += copylen;
            restlen -= copylen;
        }
    } else {
        if ((l != 0) && kr && keydataptr)
            memcpy(&(kr->data), keydataptr, l);
    }

    return (kr);
}

QVariant CRegController::getValue(struct hive *hdesc, struct vex_data vex, bool forceHex, int exact)
{
    QVariant res;
    int type = vex.type;
    int i = 0;
    const int len = vex.size;
    char *string = nullptr;
    struct keyval *kv = getKeyValue(hdesc, nullptr, vex, 0, exact);

    if (!kv) {
        qCritical() << "Value - could not fetch data" << vex.name;
        return QVariant();
    }

    void *data = (void *) & (kv->data);

    if (forceHex)
        type = REG_BINARY;

    if ((quint32) (vex.vk->len_data) == 0x80000000 && ((exact & TPF_VK_SHORT) != 0)) {
        /* Special inline case (len = 0x80000000) */
        type = REG_DWORD;
    }

    int outlen = 0;
    switch (type) {
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        string = string_regw2prog(data, len, &outlen);

        if (type == REG_MULTI_SZ) {
            for (i = 0; i < (outlen - 1); i++) {
                if (string[i] == 0)
                    string[i] = '\n';
            }
        }

            res = QString(string);
            FREE(string);
            break;

        case REG_DWORD:
            res = QVariant::fromValue((quint32) * (unsigned short *)data);
            break;

        default:
            res = QByteArray((char *)data, len);
    }

    FREE(kv);

    return res;
}

CValue CRegController::getValue(hive *hdesc, nk_key *key, const QString &name, int checkType)
{
    const QList<CValue> vl = listValues(hdesc, key);
    int idx = -1;

    if (checkType > REG_NONE) {
            idx = vl.indexOf(CValue(name, checkType));
    } else {
            for (int i = 0; i < vl.count(); i++) {
            if (vl.at(i).name == name) {
                idx = i;
                break;
            }
            }
    }

    if (idx < 0) {
        qWarning() << tr("Could not find %1 value in %2 key").arg(name, getKeyName(hdesc, key));
        return CValue();
    }

    return vl.at(idx);
}


QString CRegController::getKeyFullPath(struct hive *hdesc, struct nk_key *key, bool skipRoot)
{
    QStringList keys;
    struct nk_key *k = key;
    keys.prepend(getKeyName(hdesc, k));

    while (getKeyOfs(hdesc, k) != (hdesc->rootofs + 4)) {
        k = getKeyPtr(hdesc, k->ofs_parent + 0x1004);

        if (!checkKey(k))
            return QString();

        keys.prepend(getKeyName(hdesc, k));
    }

    if (skipRoot)
        keys.removeFirst();

    return QSL("\\%1").arg(keys.join("\\"));
}

bool CRegController::createKey(hive *hdesc, nk_key *parent, const QString &name)
{
    return (add_key(hdesc, getKeyOfs(hdesc, parent), name.toUtf8().data()) != nullptr);
}

void CRegController::deleteKey(hive *hdesc, nk_key *parent, const QString &name)
{
    rdel_keys(hdesc, name.toUtf8().data(), getKeyOfs(hdesc, parent));
}

QString escapeString(const QString &str)
{
    QString s = str;
    s.replace('\\', "\\\\");
    s.replace('\"', "\\\"");
    return s;
}

void exportBin(const QByteArray &data, int type, int &col, QTextStream &file)
{
    if (type == REG_BINARY) {
        file << "hex:";
        col += 4;
    } else {
        file << QSL("hex(%1):").arg(type, 0, 16);
        col += 7;
    }

    int start = (col  - 2) / 3;

    for (int i = 0; i < data.size(); i++) {
        if (i < (data.size() - 1)) {
            file << QSL("%1,").arg((quint8)data.at(i), 2, 16, QChar('0'));
        } else {
            file << QSL("%1").arg((quint8) data.at(i), 2, 16, QChar('0'));
        }

        if (((i + start) % 25) == 0)
            file << "\\\r\n  ";
    }

    file << "\r\n";
}

bool CRegController::exportKey(struct hive *hdesc, struct nk_key *key, const QString &prefix, QTextStream &file)
{
    const QString path = getKeyFullPath(hdesc, key, true);
    // export the key
    file << "\r\n";
    file << QSL("[%1\%2]\r\n").arg(prefix, path);
    // export values
    const QList<CValue> vl = listValues(hdesc, key);

    for (const auto &v : vl) {
        int col = 0;
        QString name = escapeString(v.name);

        /* print name */
        if (name.isEmpty()) {
            file << "@=";
            name = QSL("@");
            col += 2;
        } else {
            file << QSL("\"%1\"=").arg(name);
            col += name.length() + 3;
        }

        if (v.type == REG_DWORD)
        {
            file << QSL("dword:%1\r\n").arg(v.vDWORD, 8, 16, QChar('0'));
        }
        else if (v.type == REG_SZ)
        {
            bool hex = false;

            for (int i = 0; i < v.vString.length(); i++) {
                if (!v.vString.at(i).isPrint()) {
                    exportBin(v.vString.toUtf8(), v.type, col, file);
                    hex = true;
                    break;
                }
            }

            if (!hex)
                file << QSL("\"%1\"\r\n").arg(escapeString(v.vString));
        }
        else
        {
            exportBin(v.vOther, v.type, col, file);
        }
    }

    // export subkeys
    const QList<int> kl = listKeysOfs(hdesc, key);

    for (const auto &ofs : kl)
        exportKey(hdesc, getKeyPtr(hdesc, ofs), prefix, file);

    return true;
}

struct nk_key *CRegController::navigateKey(struct hive *hdesc, const QString &path, bool allowCreate)
{
    struct nk_key *key = getKeyPtr(hdesc, hdesc->rootofs + 4);

    QStringList kl = path.split('\\', Qt::SkipEmptyParts);

    if (kl.isEmpty())
        key = nullptr;

    while (!kl.isEmpty()) {
        const QString kn = kl.takeFirst();
        int ofs = findKeyOfs(hdesc, key, kn);

        if (ofs < 0) {
            if (allowCreate) {
                if (!createKey(hdesc, key, kn)) {
                    qCritical() << "navigateKey: failed to create key " << kn ;
                    return nullptr;
                }

                ofs = findKeyOfs(hdesc, key, kn);
            } else {
                qCritical() << "navigateKey: child not found " << kn ;
                return nullptr;
            }
        }

        key = getKeyPtr(hdesc, ofs);
    }

    return key;
}

QString CRegController::getOSInfo(struct hive *hdesc)
{
    struct nk_key *key = navigateKey(hdesc, "\\Microsoft\\Windows NT\\CurrentVersion");

    if (key == nullptr) {
        qWarning() << "\\Microsoft\\Windows NT\\CurrentVersion key not found. This is not SOFTWARE hive?";
        return QSL("\\Microsoft\\Windows NT\\CurrentVersion key not found. This is not SOFTWARE hive?");
    }

    const CValue csd = getValue(hdesc, key, "CSDVersion", REG_SZ);
    const CValue ver = getValue(hdesc, key, "CurrentVersion", REG_SZ);
    const CValue prod = getValue(hdesc, key, "ProductName", REG_SZ);
    const CValue date = getValue(hdesc, key, "InstallDate", REG_DWORD);
    const CValue id = getValue(hdesc, key, "ProductId", REG_SZ);
    const CValue build = getValue(hdesc, key, "CurrentBuildNumber", REG_SZ);

    const QDateTime idate = QDateTime::fromSecsSinceEpoch(date.vDWORD);

    QString dpids;
    const CValue dpid = getValue(hdesc, key, "DigitalProductId", REG_BINARY);

    if (dpid.isEmpty() || dpid.vOther.size() < 67) {
        qWarning() << "DigitalProductID value is too short for decoding";
        dpids = tr("(failed to decode)");
    } else {
        static const char digits[] = {'B', 'C', 'D', 'F', 'G', 'H', 'J', 'K',
            'M', 'P', 'Q', 'R', 'T', 'V', 'W', 'X',
            'Y', '2', '3', '4', '6', '7', '8', '9'
        };

        const int result_len = 24;
        const int start_offset = 52;
        const int buf_len = 15;
        QByteArray buf = dpid.vOther.mid(start_offset, buf_len);

        for (int i = result_len; i >= 0; i--) {
            unsigned int x = 0;

            for (int j = buf_len - 1; j >= 0; j--) {
                x = (x << 8) + (quint8)buf.at(j);
                buf[j] = (quint8)(x / 24);
                x = x % 24;
            }

            dpids.prepend(QChar(digits[x]));

            if ((i % 5 == 0) && (i > 0))
                dpids.prepend(QChar('-'));
        }
    }

    return QSL("%1 %2 (Version %3.%4)\n\n"
                   "Install date: %5\n\n"
                   "Product ID: %6\n"
                   "Digital Product ID: %7")
           .arg(prod.vString, csd.vString, ver.vString, build.vString)
           .arg(idate.toString(Qt::TextDate)) //WARNING .arg(idate.toString(Qt::DefaultLocaleLongDate))
           .arg(id.vString, dpids);
}

QList<CUser> CRegController::listUsers(struct hive *hdesc)
{
    QList<CUser> res;

    char SAMdaunPATH[] = "\\SAM\\Domains\\Account\\Users\\Names\\";

    int rid = 0;
    int ntpw_len = 0;

    struct keyval *m = nullptr;
    unsigned int *grps = nullptr;
    int count = 0;
    int isadmin = 0;

    unsigned short acb = 0;

    struct user_V *vpwd = nullptr;

    if (hdesc->type != HTYPE_SAM) {
        qWarning() << "This is not SAM hive";
        return res;
    }

    struct nk_key *key = navigateKey(hdesc, SAMdaunPATH);

    if (!checkKey(key)) {
        qWarning() << SAMdaunPATH << " key not found. This is not SAM hive?";
        return res;
    }

    const QStringList ul = listKeys(hdesc, key);

    for (const auto &username : ul) {
        rid = -1;

        // Extract the value out of the username-key, value is RID
        struct nk_key *ukey = navigateKey(hdesc, SAMdaunPATH + username);

        if (!checkKey(ukey)) {
            qWarning() << " Could not navigate to SAM username key for user: " << username;
            continue;
        }

        const QList<CValue> vl = listValues(hdesc, ukey, TPF_VK_EXACT | TPF_VK_SHORT);

        for (int i = 0; i < vl.count(); i++) {
            if (vl.at(i).name == QSL("@")) {
                rid = vl.at(i).type;
                break;
            }
        }

        if (rid < 0) continue;

        QByteArray vba = readVValue(hdesc, rid);

        if (!vba.isEmpty()) {
            vpwd = (struct user_V *)vba.data();
            ntpw_len = vpwd->ntpw_len;

            acb = sam_handle_accountbits(hdesc, rid, 0);

            // check user's groups
            m = sam_get_user_grpids(hdesc, rid);
            QList<int> groups;
            groups.clear();
            isadmin = 0;

            if (m) {
                grps = (unsigned int *)&m->data;
                count = m->len >> 2;

                for (int i = 0; i < count; i++) {
                    if (grps[i] == 0x220) isadmin = 1;

                    groups << grps[i];
                }

                free(m);
            }

            CUser us = CUser(rid, username, isadmin != 0, (acb & 0x8000) != 0, (ntpw_len < 16));
            us.groupIDs.append(groups);

            int vlen = vba.size();

            int username_offset = vpwd->username_ofs;
            int username_len    = vpwd->username_len;
            int fullname_offset = vpwd->fullname_ofs;
            int fullname_len    = vpwd->fullname_len;
            int comment_offset  = vpwd->comment_ofs;
            int comment_len     = vpwd->comment_len;
            int homedir_offset  = vpwd->homedir_ofs;
            int homedir_len     = vpwd->homedir_len;
            int profile_offset  = vpwd->profilep_ofs;
            int profile_len     = vpwd->profilep_len;
            int drvletter_offset = vpwd->drvletter_ofs;
            int drvletter_len   = vpwd->drvletter_len;
            int logonscr_offset = vpwd->logonscr_ofs;
            int logonscr_len    = vpwd->logonscr_len;

            if (username_len <= 0 || username_len > vlen ||
                    comment_len < 0 || comment_len > vlen   ||
                    fullname_len < 0 || fullname_len > vlen ||
                    homedir_len < 0 || homedir_len > vlen ||
                    profile_len < 0 || profile_len > vlen ||
                    drvletter_len < 0 || drvletter_len > vlen ||
                    logonscr_len < 0 || logonscr_len > vlen ||
                    username_offset <= 0 || username_offset >= vlen ||
                    fullname_offset < 0 || fullname_offset >= vlen ||
                    comment_offset < 0 || comment_offset >= vlen ||
                    homedir_offset < 0 || homedir_offset >= vlen ||
                    profile_offset < 0 || profile_offset >= vlen ||
                    drvletter_offset < 0 || drvletter_offset >= vlen ||
                    logonscr_offset < 0 || logonscr_offset >= vlen)
            {
                qCritical() << "getUserInfo: Not a legal V struct? (negative struct lengths)";
            } else {

                // Offsets in top of struct is relative to end of pointers, adjust
                username_offset += 0xCC;
                fullname_offset += 0xCC;
                comment_offset += 0xCC;
                homedir_offset += 0xCC;
                profile_offset += 0xCC;
                drvletter_offset += 0xCC;
                logonscr_offset += 0xCC;

                // leave username as from V-key name
                us.fullname = fromUtf16(vba.mid(fullname_offset, fullname_len));
                us.comment  = fromUtf16(vba.mid(comment_offset, comment_len));
                us.homeDir  = fromUtf16(vba.mid(homedir_offset, homedir_len));
                us.profilePath = fromUtf16(vba.mid(profile_offset, profile_len));
                us.driveLetter = fromUtf16(vba.mid(drvletter_offset, drvletter_len));
                us.logonScript = fromUtf16(vba.mid(logonscr_offset, logonscr_len));
            }

            res << us;
        }
    }

    return res;
}

QList<CGroup> CRegController::listGroups(struct hive *hdesc)
{
    struct sid_array *sids = nullptr;
    unsigned int grp = 0;
    struct group_C *cd = nullptr;
    char *str = nullptr;
    char *username = nullptr;

    QList<CGroup> res;

    if (hdesc->type != HTYPE_SAM) return res;

    // Paths for group ID
    static const QStringList grpcPaths({ QSL("\\SAM\\Domains\\Builtin\\Aliases"),
                                        QSL("\\SAM\\Domains\\Account\\Aliases") });

    for (const auto &grpcPath : grpcPaths) {
        struct nk_key *key = navigateKey(hdesc, grpcPath);

        if (!checkKey(key)) {
            qWarning() << grpcPath << " key not found. This is not SAM hive?";
            return res;
        }

        const QStringList grpsl = listKeys(hdesc, key);

        for (const auto &grps : grpsl) {
            /* Pick up all subkeys here, they are local groups */

            bool ok = false;
            grp = grps.toInt(&ok, 16);

            if (!ok) continue;

            /* Groups keys have a C value, get it and pick up the name etc */
            /* Some other keys also exists (Members, Names at least), but we skip them */

            struct nk_key *gkey = navigateKey(hdesc, QSL("%1\\%2").arg(grpcPath, grps));

            if (!checkKey(gkey)) {
                qWarning() << grps << "key not found in" << grpcPath << "key. Really strange...";
                return res;
            }

            const QList<CValue> vl = listValues(hdesc, gkey, TPF_VK_EXACT);
            int vidx = vl.indexOf(CValue("C", REG_BINARY));

            if (vidx < 0) {
                qWarning() << " Could not locate C-key in SAM for group: " << grps << grpcPath;
                continue;
            }

            CValue v = vl.at(vidx);

            if (!v.vOther.isEmpty()) {
                cd = (struct group_C *)v.vOther.data();

                CGroup group(grp,
                             fromUtf16(v.vOther.mid(cd->grpname_ofs + 0x34, cd->grpname_len)),
                             fromUtf16(v.vOther.mid(cd->fullname_ofs + 0x34, cd->fullname_len)));

                sam_get_grp_members_sid(hdesc, grp, &sids);

                QList<CGroupMember> members;

                for (int i = 0; sids[i].sidptr; i++) {
                    str = sam_sid_to_string(sids[i].sidptr);
                    username = sam_get_username_from_sid(hdesc, sids[i].sidptr);
                    members << CGroupMember(sids[i].sidptr->array[sids[i].sidptr->sections - 1],
                                            QString::fromUtf8(username),
                                            QString::fromLatin1(str));

                    FREE(username);
                    FREE(str);
                }

                sam_free_sid_array(sids);

                group.members.append(members);
                res << group;
            }
        }
    }

    return res;
}

QByteArray CRegController::readFValue(struct hive *hdesc, int rid)
{
    struct nk_key *key = navigateKey(hdesc, QSL("\\SAM\\Domains\\Account\\Users\\") +
                                     QSL("%1").arg((quint16)rid, 8, 16, QChar('0')).toUpper());

    if (!checkKey(key)) {
        qWarning() << "F-value not found. This is not SAM hive.";
        return QByteArray();
    }

    const QList<CValue> vl = listValues(hdesc, key, TPF_VK_EXACT);
    int vidx = vl.indexOf(CValue("F", REG_BINARY));

    if (vidx < 0) {
        qWarning() << "Could not locate F-value in SAM for user.";
        return QByteArray();
    }

    CValue v = vl.at(vidx);

    if (v.vOther.length() < 0x48) {
        qWarning() << tr("F-value is 0x%1 bytes sized, need >= 0x48, unable to check account flags.")
                   .arg((quint16)v.vOther.length(), 8, 16, QChar('0'));
        return QByteArray();
    }

    return v.vOther;
}

bool CRegController::writeFValue(struct hive *hdesc, int rid, const QByteArray &f)
{
    struct nk_key *key = navigateKey(hdesc, QSL("\\SAM\\Domains\\Account\\Users\\") +
                                     QSL("%1").arg((quint16)rid, 8, 16, QChar('0')).toUpper());

    if (!checkKey(key)) {
        qWarning() << "F-value not found. This is not SAM hive.";
        return false;
    }

    CValue v = CValue("F", REG_BINARY);
    v.vOther = f;
    return setValue(hdesc, key, v);
}

QByteArray CRegController::readVValue(hive *hdesc, int rid)
{
    const QString keynm = QSL("\\SAM\\Domains\\Account\\Users\\") +
                          QSL("%1").arg((quint16)rid, 8, 16, QChar('0')).toUpper();
    struct nk_key *ukey = navigateKey(hdesc, keynm);

    if (!checkKey(ukey)) {
        qWarning() << " Could not navigate to SAM username key for user " << keynm;
        return QByteArray();
    }

    const QList<CValue> vl = listValues(hdesc, ukey, TPF_VK_EXACT | TPF_VK_SHORT);
    int vidx = vl.indexOf(CValue("V", REG_BINARY));

    if (vidx < 0) {
        qWarning() << " Could not locate V-key in SAM for user " << keynm;
        return QByteArray();

    }

    CValue v = vl.at(vidx);

    if (v.vOther.length() < 0xcc) {
        qWarning() << tr("V-value for user (rid: %1) is too short (only %2 bytes) "
                         "to be a SAM user V-struct!")
                   .arg(rid).arg(v.vOther.size());
        return QByteArray();
    }

    return v.vOther;
}

bool CRegController::writeVValue(hive *hdesc, int rid, const QByteArray &data)
{
    const QString keynm = QSL("\\SAM\\Domains\\Account\\Users\\") +
                          QSL("%1").arg((quint16)rid, 8, 16, QChar('0')).toUpper();
    struct nk_key *ukey = navigateKey(hdesc, keynm);

    if (!checkKey(ukey)) {
        qWarning() << " Could not navigate to SAM username key for user " << keynm;
        return false;
    }

    CValue v = CValue("V", REG_BINARY);
    v.vOther = data;
    return setValue(hdesc, ukey, v);
}

QString unquoteString(const QString &s)
{
    QString a = s;

    if (!s.startsWith('"')) return a;

    a.remove(0, 1);
    a.remove(a.length() - 1, 1);
    return a;
}

QString unescapeString(const QString &s)
{
    QString a = s;
    a.replace("\\\"", "\"");
    a.replace("\\\\", "\\");
    return a;
}

CValue parseValueStr(const QString &s)
{
    QString name;
    QString val;
    CValue v;

    // search for '=' outside quotes
    bool q = false;

    for (int i = 0; i < s.length(); i++) {
        if (s.at(i) == QChar('"')) q ^= 1;

        if ((s.at(i) == QChar('=')) && !q) {
            name = unescapeString(unquoteString(s.left(i)));
            val = unescapeString(unquoteString(s.mid(i + 1)));
            break;
        }
    }

    if (name.isEmpty()) {
        qCritical() << "parseValueStr: empty value name parsed";
        return CValue();
    }

    if (val.startsWith("dword")) {
        v = CValue(REG_DWORD);
        val.remove(0, val.indexOf(':') + 1);
        bool ok = false;
        v.vDWORD = val.toUInt(&ok, 16);

        if (!ok) {
            qCritical() << "parseValueStr: incorrect hex dword value for " << v.name;
            return CValue();
        }
    } else if (val.startsWith("hex")) {
        int type = REG_BINARY;
        QString vt = val.mid(3, val.indexOf(':') - 3);

        if (!vt.isEmpty()) {
            vt.remove(0, 1);
            vt.remove(vt.length() - 1, 1);
            bool ok = false;
            type = vt.toInt(&ok, 16);

            if (!ok) {
                qCritical() << "parseValueStr: failed to parse value type " << v.name;
                return CValue();
            }
        }

        v = CValue(type);
        val.remove(0, val.indexOf(':') + 1);

        while (!val.isEmpty()) {
            bool ok = false;
            v.vOther.append((quint8)val.left(2).toInt(&ok, 16));

            if (!ok) {
                qCritical() << "parseValueStr: failed to parse hex value " << v.name;
                return CValue();
            }

            val.remove(0, 2);

            if (val.startsWith(','))
                val.remove(0, 1);

        }

        if (v.type == REG_EXPAND_SZ ||
                v.type == REG_MULTI_SZ) {
            if (v.vOther.endsWith(QByteArray(2, 0))) // remove trailing \0
                v.vOther.remove(v.vOther.size() - 2, 2);

            v.vString = fromUtf16(v.vOther);

            if (v.type == REG_MULTI_SZ)
                v.vString.replace(QChar(0), QChar('\n'));
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
    const QString prefix = getHivePrefix(hdesc);
    const QString sign = fs.readLine();

    if (!sign.startsWith("Windows Registry Editor Version 5.00")) {
        qCritical() << "importReg: file signature missing";
        return false;
    }

    struct nk_key *key = nullptr;

    QString valacc;

    while (!fs.atEnd()) {
        QString s = fs.readLine().trimmed();

        if (s.startsWith('[')) { // keyname
            s.remove(0, 1);
            s.remove(s.length() - 1, 1);

            if (!s.startsWith(prefix, Qt::CaseInsensitive)) {
                qCritical() << tr("importReg: could not import key %1 to %2 registry hive").arg(s, prefix);
                return false;
            }

            s.remove(prefix, Qt::CaseInsensitive);
            key = navigateKey(hdesc, s, true);

            if (key == nullptr) {
                qCritical() << "importReg: failed to navigate key " << s;
                return false;
            }

            valacc.clear();
        } else if (!s.isEmpty()) {
            if (s.endsWith('\\')) { // string with wrap marker
                s.remove(s.length() - 1, 1);
                valacc += s;
                continue;
            }

            s = valacc + s; // Now we have complete value string

            CValue v = parseValueStr(s);

            if (v.isEmpty()) {
                qCritical() << "importReg: failed to parse value " << s;
                return false;
            }

            const QList<CValue> vl = listValues(hdesc, key);

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
    auto k = (quintptr) (key);

    for (int i = 0; i < hives.count(); i++) {
        auto hive = (quintptr) (hives.at(i)->buffer);
        const quintptr size = hives.at(i)->size;

        if ((k >= hive) && (k < (hive + size)))
            return i;
    }

    return -1;
}

bool CRegController::checkKey(const nk_key *key) const
{
    if (getHive(key) == -1) {
        qCritical() << tr("Error: 'nk' not assigned to opened hives, at offset 0x%1!\n").arg((quintptr)key);
        return false;
    }

    if (key->id != 0x6b6e) {
        qCritical() << tr("Error: Not a 'nk' node at offset 0x%0x!\n").arg((quintptr)key);
        return false;
    }

    return true;
}

bool CRegController::keyPrepare(const void *ptr, struct hive *&hive, int &hnum, struct nk_key *&key) const
{
    key = (struct nk_key *)ptr;

    if (!checkKey(key))
        return false;

    hnum = getHive(key);

    if (hnum == -1)
        return false;

    hive = hives.at(hnum);
    return true;
}

QString CRegController::getValueTypeStr(int type)
{
    if (type >= REG_NONE && type < REG_MAX) {
        return QString(val_types[type]);
    }
    return QSL("REG_UNKNOWN (0x%1)").arg((quint16) type, 0, 16);
}

bool CRegController::setValue(struct hive *hdesc, struct nk_key *key, const CValue &value)
{
    struct keyval *newkv = nullptr;
    int newsize = 0;
    QByteArray str;
    QString s = value.vString;

    switch (value.type) {
        case REG_DWORD:
            newkv = getKeyValue(hdesc, key, nullptr, value.name, value.type, TPF_VK);
            newkv->data = value.vDWORD;
            break;

        case REG_MULTI_SZ:
            if (value.type == REG_MULTI_SZ)
                s.replace(QChar('\n'), QChar(0));
            [[fallthrough]];

        case REG_SZ:
        case REG_EXPAND_SZ:
            str = toUtf16(s);
            newsize = str.size();
            newkv = (struct keyval *)calloc(1, newsize + sizeof(int));
            newkv->len = newsize;
            memcpy((char *) & (newkv->data), str.data(), newsize);
            break;

        default: // blob
            newsize = value.vOther.size();
            newkv = (struct keyval *)calloc(1, newsize + sizeof(int) +4);
            memcpy((char *) & (newkv->data), value.vOther.data(), value.vOther.size());
            newkv->len = newsize;
            break;
    }

    if (newkv != nullptr) {
        bool res = put_buf2val(hdesc, newkv, getKeyOfs(hdesc, key),
                               value.name.toUtf8().data(), value.type, TPF_VK_EXACT) >= 0;
        FREE(newkv);
        return res;
    }

    return false;

}

bool CRegController::deleteValue(struct hive *hdesc, struct nk_key *key, const QString &vname)
{
    return del_value(hdesc, getKeyOfs(hdesc, key), vname.toUtf8().data(), TPF_EXACT) == 0;
}

bool CRegController::createValue(struct hive *hdesc, struct nk_key *key, int vtype, const QString &vname)
{
    return add_value(hdesc, getKeyOfs(hdesc, key), vname.toUtf8().data(), vtype) != nullptr;
}

CValue::CValue(int atype)
    : type(atype)
{}

CValue::CValue(const QString &aname, int atype)
    : type(atype)
    , name(aname)
{}

CValue::CValue(struct vex_data vex, const QString &str, const QByteArray &data)
{
    name = QString(vex.name);

    if (name.isEmpty())
        name = QSL("@");

    type = vex.type;
    vDWORD = vex.val;
    vString = str;
    vOther = data;
}

bool CValue::operator==(const CValue &ref) const
{
    return ((ref.name == name) && (ref.type == type));
}

bool CValue::operator!=(const CValue &ref) const
{
    return !operator ==(ref);
}

bool CValue::isEmpty() const
{
    return (name.isEmpty() && type == REG_NONE);
}

bool CValue::isDefault() const
{
    return ((name.isEmpty() || name == QSL("@")) && !isEmpty());
}

CUser::CUser(int arid)
    : rid(arid)
{}

CUser::CUser(int arid, const QString &ausername, bool admin, bool locked, bool blank_pw)
    : rid(arid)
    , is_admin(admin)
    , is_locked(locked)
    , is_blank_pw(blank_pw)
    , username(ausername)
{}

bool CUser::operator==(const CUser &ref) const
{
    return (rid == ref.rid);
}

bool CUser::operator!=(const CUser &ref) const
{
    return !operator ==(ref);
}

bool CUser::isEmpty() const
{
    return (rid == 0);
}

CGroup::CGroup(int id)
    : grpid(id)
{}

CGroup::CGroup(int id, const QString &aname, const QString &afullname)
    : grpid(id)
    , name(aname)
    , fullname(afullname)
{}

bool CGroup::operator==(const CGroup &ref) const
{
    return (grpid == ref.grpid);
}

bool CGroup::operator!=(const CGroup &ref) const
{
    return !operator ==(ref);
}

bool CGroup::isEmpty() const
{
    return (grpid < 0 && name.isEmpty());
}

CGroupMember::CGroupMember(int arid, const QString &aname, const QString &asid)
    : rid(arid)
    , name(aname)
    , sid(asid)
{}

bool CGroupMember::operator==(const CGroupMember &ref) const
{
    return (rid == ref.rid && name == ref.name && sid == ref.sid);
}

bool CGroupMember::operator!=(const CGroupMember &ref) const
{
    return !operator ==(ref);
}

bool CGroupMember::isEmpty() const
{
    return (name.isEmpty() && sid.isEmpty() && rid < 0);
}
