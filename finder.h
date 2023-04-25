#ifndef CFINDER_H
#define CFINDER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QModelIndex>

struct hive;
struct nk_key;

class CFinder : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CFinder)

private:
    int searchHive { -1 };
    int searchLastKeyIdx { -1 };
    bool m_canceled { false };
    QList<int> searchKeysOfsFlat;
    QString searchString;

public:
    explicit CFinder(QObject *parent = nullptr);
    ~CFinder() override;
    void hiveChanged(const QModelIndex &idx);
    void cancelSearch() { m_canceled = true; }

public Q_SLOTS:
    void searchText(const QModelIndex& idx, const QString& text);
    void continueSearch();

Q_SIGNALS:
    void keyFound(struct hive *hdesc, struct nk_key *key, const QString &value);
    void searchFinished();

    void showProgressDialog();
    void hideProgressDialog();
};

#endif // CFINDER_H
