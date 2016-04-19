#include <QSettings>
#include "global.h"
#include "settingsdlg.h"
#include "ui_settingsdlg.h"

CGlobal* cgl = NULL;

CGlobal::CGlobal(QObject *parent) : QObject(parent)
{
    hiveOpenMode = 0;
    reg = new CRegController(this);

    loadSettings();
}

bool CGlobal::safeToClose(int idx)
{
    if (idx>=0 && idx<reg->getHivesCount()) {
        if (!reg->saveTopHive(idx))
            return false;
        reg->closeTopHive(idx);
        return true;
    }

    writeSettings();

    for (int i=0;i<reg->getHivesCount();i++)
        if (!reg->saveTopHive(i))
            return false;

    while (reg->getHivesCount()>0)
        reg->closeTopHive(0);

    return true;
}

void CGlobal::loadSettings()
{
    QSettings settings("kernel1024","qregedit");
    settings.beginGroup("Main");
    hiveOpenMode = settings.value("hiveOpenMode",0).toInt();
    settings.endGroup();
}

void CGlobal::writeSettings()
{
    QSettings settings("kernel1024","qregedit");
    settings.beginGroup("Main");
    settings.remove("");
    settings.setValue("hiveOpenMode",hiveOpenMode);
    settings.endGroup();
}

void CGlobal::settingsDialog(QWidget *parent)
{
    CSettingsDlg* dlg = new CSettingsDlg(parent);
    dlg->ui->checkNoAlloc->setChecked(hiveOpenMode & HMODE_NOALLOC);
    dlg->ui->checkNoExpand->setChecked(hiveOpenMode & HMODE_NOEXPAND);

    if (dlg->exec()==QDialog::Accepted) {
        if (dlg->ui->checkNoExpand->isChecked())
            hiveOpenMode |= HMODE_NOEXPAND;
        else
            hiveOpenMode &= ~HMODE_NOEXPAND;

        if (dlg->ui->checkNoAlloc->isChecked())
            hiveOpenMode |= HMODE_NOALLOC;
        else
            hiveOpenMode &= ~HMODE_NOALLOC;
    }
    dlg->deleteLater();
}
