#ifndef REGUTILS_H
#define REGUTILS_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QPointer>
#include <QAbstractItemModel>
#include <QTextStream>

extern "C" {
#include <chntpw/ntreg.h>
#include <chntpw/sam.h>
}

class CRegistryModel;
class CValuesModel;

using CStrHash = QHash<QString,QString>;

class CValue
{
public:
    int type { REG_NONE };
    quint32 vDWORD { 0 };
    QString name;
    QString vString;
    QByteArray vOther;

    CValue() = default;
    virtual ~CValue() = default;
    CValue(const CValue& other) = default;
    explicit CValue(int atype);
    CValue(const QString& aname, int atype);
    CValue(struct vex_data vex, const QString &str, const QByteArray &data);
    CValue &operator=(const CValue& other) = default;
    bool operator==(const CValue& ref) const;
    bool operator!=(const CValue& ref) const;
    bool isEmpty() const;
    bool isDefault() const;
};

Q_DECLARE_METATYPE(CValue)

class CUser
{
public:
    int rid { 0 };
    bool is_admin { false };
    bool is_locked { false };
    bool is_blank_pw { false };
    QString username;
    QString fullname;
    QString comment;
    QString homeDir;
    QString profilePath;
    QString driveLetter;
    QString logonScript;
    QList<int> groupIDs;

    CUser() = default;
    virtual ~CUser() = default;
    CUser(const CUser& other) = default;
    explicit CUser(int arid);
    CUser(int arid, const QString& ausername, bool admin, bool locked, bool blank_pw);
    CUser &operator=(const CUser& other) = default;
    bool operator==(const CUser& ref) const;
    bool operator!=(const CUser& ref) const;
    bool isEmpty() const;
};

Q_DECLARE_METATYPE(CUser)

class CGroupMember
{
public:
    int rid { -1 };
    QString name;
    QString sid;

    CGroupMember() = default;
    virtual ~CGroupMember() = default;
    CGroupMember(const CGroupMember& other) = default;
    CGroupMember(int arid, const QString& aname, const QString& asid);
    CGroupMember &operator=(const CGroupMember& other) = default;
    bool operator==(const CGroupMember& ref) const;
    bool operator!=(const CGroupMember& ref) const;
    bool isEmpty() const;
};

Q_DECLARE_METATYPE(CGroupMember)

class CGroup
{
public:
    int grpid { -1 };
    QString name;
    QString fullname;
    QList<CGroupMember> members;

    CGroup() = default;
    virtual ~CGroup() = default;
    CGroup(const CGroup& other) = default;
    explicit CGroup(int id);
    CGroup(int id, const QString& aname, const QString& afullname);
    CGroup &operator=(const CGroup& other) = default;
    bool operator==(const CGroup& ref) const;
    bool operator!=(const CGroup& ref) const;
    bool isEmpty() const;
};

Q_DECLARE_METATYPE(CGroup)


class CRegController : public QObject
{
    Q_OBJECT
private:
    QList <struct hive *> hives;

public:
    QPointer<CRegistryModel> treeModel; // TODO: hide this
    QPointer<CValuesModel> valuesModel;

    explicit CRegController(QObject* parent = nullptr);

    bool openTopHive(const QString &filename, int mode);
    bool saveTopHive(int idx);
    void closeTopHive(int idx);

    int getHivesCount() const { return hives.count(); }
    struct hive* getHivePtr(int idx);
    int getHive(const struct nk_key * key) const;
    bool checkKey(const struct nk_key * key) const;
    bool keyPrepare(const void *ptr, struct hive *&hive, int &hnum, struct nk_key *&key) const;

    QStringList listKeys(struct hive *hdesc, nk_key *key);
    QList<int> listKeysOfs(struct hive *hdesc, nk_key *key);
    QList<int> listAllKeysOfsFlat(struct hive *hdesc, nk_key *key);
    struct nk_key * getKeyPtr(struct hive* hdesc, int nkofs);
    int getKeyOfs(struct hive* hdesc, struct nk_key* key);
    QString getKeyName(struct hive *hdesc, struct nk_key* key);
    QString getKeyTooltip(struct hive *hdesc, struct nk_key* key);
    QString getKeyFullPath(struct hive *hdesc, struct nk_key* key, bool skipRoot = false);
    bool createKey(struct hive *hdesc, struct nk_key* parent, const QString& name);
    void deleteKey(struct hive *hdesc, struct nk_key* parent, const QString& name);
    bool exportKey(struct hive *hdesc, struct nk_key* key, const QString &prefix, QTextStream &file);
    bool importReg(struct hive *hdesc, const QString& filename);

    QVariant getValue(struct hive *hdesc, struct vex_data vex, bool forceHex, int exact = TPF_VK);
    CValue getValue(struct hive *hdesc, struct nk_key *key, const QString& name, int checkType = REG_NONE);
    QList<CValue> listValues(struct hive *hdesc, struct nk_key *key, int exact = TPF_VK);
    struct keyval *getKeyValue(struct hive *hdesc, struct keyval *kv, const struct vex_data &vex,
                               int type, int exact);
    struct keyval *getKeyValue(struct hive *hdesc, struct nk_key *key, struct keyval *kv,
                               const QString &name, int type, int exact);
    QString getValueTypeStr(int type);
    bool setValue(struct hive *hdesc, struct nk_key *key, const CValue& value);
    bool deleteValue(struct hive *hdesc, struct nk_key *key, const QString& vname);
    bool createValue(struct hive *hdesc, struct nk_key *key, int vtype, const QString& vname);
    QString getHivePrefix(struct hive *hdesc);
    int findKeyOfs(struct hive *hdesc, struct nk_key *key, const QString &name);
    struct nk_key *navigateKey(struct hive *hdesc, const QString &path, bool allowCreate=false);

    // For SOFTWARE hive
    QString getOSInfo(struct hive *hdesc);

    // For SAM hive
    QList<CUser> listUsers(struct hive *hdesc);
    QList<CGroup> listGroups(struct hive *hdesc);
    QByteArray readFValue(struct hive *hdesc, int rid);
    bool writeFValue(struct hive *hdesc, int rid, const QByteArray &f);
    QByteArray readVValue(struct hive *hdesc, int rid);
    bool writeVValue(struct hive *hdesc, int rid, const QByteArray &data);
Q_SIGNALS:
    void hiveOpened(int idx);
    void hiveClosed(int old_idx);
    void hiveSaved(int idx);
    void hiveAboutToClose(int idx);
};

QByteArray toUtf16(const QString &str);
QString fromUtf16(const QByteArray &str);

#endif // REGUTILS_H
