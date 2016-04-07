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

class CRegController : public QObject
{
private:
    QVector<struct hive *> hives;

public:
    QPointer<CRegistryModel> treeModel;

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

};

#endif // REGUTILS_H
