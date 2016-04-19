#include "settingsdlg.h"
#include "ui_settingsdlg.h"

CSettingsDlg::CSettingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSettingsDlg)
{
    ui->setupUi(this);
}

CSettingsDlg::~CSettingsDlg()
{
    delete ui;
}
