#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>

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
    int m_hive;


    void reloadUserInfo();
};

#endif // USERDIALOG_H
