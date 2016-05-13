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

public:
    explicit CUserDialog(QWidget *parent, int hive_idx, int rid);
    ~CUserDialog();

private:
    Ui::CUserDialog *ui;
    int m_rid;
    CUser* m_user;
    struct hive* m_hive;

    void reloadUserInfo();

public slots:
    void unlockAccount();
    void promoteUser();
};

#endif // USERDIALOG_H
