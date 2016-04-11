#include <QSettings>
#include "global.h"

CGlobal* cgl = NULL;

CGlobal::CGlobal(QObject *parent) : QObject(parent)
{
    hiveOpenMode = 0;
    reg = new CRegController(this);

    loadSettings();
}

bool CGlobal::safeToClose()
{
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
