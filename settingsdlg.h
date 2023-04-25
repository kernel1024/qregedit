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
    Q_DISABLE_COPY(CSettingsDlg)

    friend class CGlobal;
public:
    explicit CSettingsDlg(QWidget *parent = nullptr);
    ~CSettingsDlg() override;

private:
    Ui::CSettingsDlg *ui;
};

#endif // SETTINGSDLG_H
