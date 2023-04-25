#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class CUserDialog;
}

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
    CUser* m_user { nullptr }; // TODO: manage
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
