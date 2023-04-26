#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
class CUserDialog;
}

class CUser;

class CUserDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(CUserDialog)

public:
    CUserDialog(QWidget *parent, int hive_idx, int rid);
    ~CUserDialog() override;

private:
    Ui::CUserDialog *ui;
    int m_rid { 0 };
    QScopedPointer<CUser> m_user;
    struct hive* m_hive { nullptr };

    void reloadUserInfo();

public Q_SLOTS:
    void unlockAccount();
    void promoteUser();
    void clearPassword();
    void addToGroup();
    void removeFromGroup();
};

#endif // USERDIALOG_H
