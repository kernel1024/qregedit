#ifndef CFINDER_H
#define CFINDER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QModelIndex>
#include "progressdialog.h"

struct hive;
struct nk_key;

class CFinder : public QObject
{
    Q_OBJECT
private:
    int searchHive;
    QList<int> searchKeysOfsFlat;
    int searchLastKeyIdx;
    QString searchString;
    bool m_canceled;

public:
    explicit CFinder(QObject *parent = 0);
    void hiveChanged(const QModelIndex &idx);
    void cancelSearch() { m_canceled = true; }

public slots:
    void searchText(const QModelIndex& idx, const QString& text);
    void continueSearch();

signals:
    void keyFound(struct hive *hdesc, struct nk_key *key, const QString &value);
    void searchFinished();

    void showProgressDialog();
    void hideProgressDialog();
};

#endif // CFINDER_H
