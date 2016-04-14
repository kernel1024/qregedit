#ifndef REGUTILS_H
#define REGUTILS_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QList>
#include <QPointer>
#include <QAbstractItemModel>

extern "C" {
#include <chntpw/ntreg.h>
}

class CRegistryModel;
class CValuesModel;

class CValue
{
public:
    QString name;
    int type;
    int size;
    int vDWORD;
    QString vString;
    QByteArray vOther;
    struct vex_data m_vex;
    CValue();
    CValue(int atype);
    CValue(struct vex_data vex, const QString &str, const QByteArray &data);
    CValue &operator=(const CValue& other);
    bool operator==(const CValue& ref) const;
    bool operator!=(const CValue& ref) const;
    bool isEmpty() const;
    bool isDefault() const;
};

Q_DECLARE_METATYPE(CValue)

class CRegController : public QObject
{
    Q_OBJECT
private:
    QVector<struct hive *> hives;

public:
    QPointer<CRegistryModel> treeModel;
    QPointer<CValuesModel> valuesModel;

    CRegController(QObject* parent = 0);

    QByteArray toUtf16(const QString &str);

    bool openTopHive(const QString &filename);
    bool saveTopHive(int idx);
    void closeTopHive(int idx);

    int getHivesCount() const { return hives.count(); }
    struct hive* getHivePtr(int idx) { return hives.at(idx); }
    int getHive(const struct nk_key * key) const;
    bool checkKey(const struct nk_key * key) const;
    bool keyPrepare(const void *ptr, struct hive *&hive, int &hnum, struct nk_key *&key) const;

    QStringList listKeys(struct hive *hdesc, nk_key *key);
    QList<int> listKeysOfs(struct hive *hdesc, nk_key *key);
    struct nk_key * getKeyPtr(struct hive* hdesc, int nkofs);
    int getKeyOfs(struct hive* hdesc, struct nk_key* key);
    QString getKeyName(struct hive *hdesc, struct nk_key* key);
    QString getKeyTooltip(struct hive *hdesc, struct nk_key* key);
    QString getKeyFullPath(struct hive *hdesc, struct nk_key* key);
    bool createKey(struct hive *hdesc, struct nk_key* parent, const QString& name);
    void deleteKey(struct hive *hdesc, struct nk_key* parent, const QString& name);

    QVariant getValue(struct hive *hdesc, struct vex_data vex, int forceHex);
    QList<CValue> listValues(struct hive *hdesc, struct nk_key *key);
    struct keyval *getKeyValue(struct hive *hdesc, struct keyval *kv, struct vex_data vex,
                               int type, int exact);
    struct keyval *getKeyValue(struct hive *hdesc, struct nk_key *key, struct keyval *kv,
                               const QString &name, int type, int exact);
    QString getValueTypeStr(int type);
    bool setValue(struct hive *hdesc, struct nk_key *key, const CValue& value);
    bool deleteValue(struct hive *hdesc, struct nk_key *key, const QString& vname);
    bool createValue(struct hive *hdesc, struct nk_key *key, int vtype, const QString& vname);
signals:
    void hiveOpened(int idx);
    void hiveClosed(int old_idx);
    void hiveSaved(int idx);
    void hiveAboutToClose(int idx);
};

#endif // REGUTILS_H
