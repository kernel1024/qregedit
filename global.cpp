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

QString getOpenFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileName(parent,caption,dir,filter,selectedFilter,options);
}

QStringList getOpenFileNamesD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileNames(parent,caption,dir,filter,selectedFilter,options);
}

QStringList getSuffixesFromFilter(const QString& filter)
{
    QStringList res;
    res.clear();
    if (filter.isEmpty()) return res;

    res = filter.split(";;",QString::SkipEmptyParts);
    if (res.isEmpty()) return res;
    QString ex = res.first();
    res.clear();

    if (ex.isEmpty()) return res;

    ex.remove(QRegExp("^.*\\("));
    ex.remove(QRegExp("\\).*$"));
    ex.remove(QRegExp("^.*\\."));
    res = ex.split(" ");

    return res;
}

QString getSaveFileNameD ( QWidget * parent, const QString & caption, const QString & dir, const QString & filter, QString * selectedFilter, QFileDialog::Options options, QString preselectFileName )
{
    QFileDialog d(parent,caption,dir,filter);
    d.setFileMode(QFileDialog::AnyFile);
    d.setOptions(options);
    d.setAcceptMode(QFileDialog::AcceptSave);

    QStringList sl;
    if (selectedFilter!=NULL && !selectedFilter->isEmpty())
        sl=getSuffixesFromFilter(*selectedFilter);
    else
        sl=getSuffixesFromFilter(filter);
    if (!sl.isEmpty())
        d.setDefaultSuffix(sl.first());

    if (selectedFilter && !selectedFilter->isEmpty())
            d.selectNameFilter(*selectedFilter);

    if (!preselectFileName.isEmpty())
        d.selectFile(preselectFileName);

    if (d.exec()==QDialog::Accepted) {
        if (selectedFilter!=NULL)
            *selectedFilter=d.selectedNameFilter();
        if (!d.selectedFiles().isEmpty())
            return d.selectedFiles().first();
        else
            return QString();
    } else
        return QString();
}

QString	getExistingDirectoryD ( QWidget * parent, const QString & caption, const QString & dir, QFileDialog::Options options )
{
    return QFileDialog::getExistingDirectory(parent,caption,dir,options);
}
