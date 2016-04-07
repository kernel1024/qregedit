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
    CValue();
    CValue(struct vex_data vex, const QString &str, const QByteArray &data);
    CValue &operator=(const CValue& other);
    bool operator==(const CValue& ref) const;
    bool operator!=(const CValue& ref) const;
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

    bool openTopHive(const QString &filename);

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
    QVariant getValue(hive *hdesc, vex_data vex, int forceHex);
    QList<CValue> listValues(hive *hdesc, nk_key *key);
};

#endif // REGUTILS_H
