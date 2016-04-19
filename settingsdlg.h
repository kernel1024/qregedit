#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class CSettingsDlg;
}

class CSettingsDlg : public QDialog
{
    Q_OBJECT

    friend class CGlobal;
public:
    explicit CSettingsDlg(QWidget *parent = 0);
    ~CSettingsDlg();

private:
    Ui::CSettingsDlg *ui;
};

#endif // SETTINGSDLG_H
