#include <QSettings>
#include <QMutex>
#include <QTime>
#include <QRegularExpression>
#include "global.h"
#include "settingsdlg.h"
#include "ui_settingsdlg.h"

QPointer<CGlobal> cgl;

void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex loggerMutex;

    loggerMutex.lock();

    QString lmsg = QString();

    const int line = context.line;
    const QString file = QString(context.file);
    QString category = QString(context.category);

    if (category == QSL("default")) {
        category.clear();
    } else {
        category.append(' ');
    }

    switch (type) {
    case QtDebugMsg:
        lmsg = QSL("%1Debug: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
        break;

    case QtWarningMsg:
        lmsg = QSL("%1Warning: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
        break;

    case QtCriticalMsg:
        lmsg = QSL("%1Critical: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
        break;

    case QtFatalMsg:
        lmsg = QSL("%1Fatal: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
        break;

    case QtInfoMsg:
        lmsg = QSL("%1Info: %2 (%3:%4)").arg(category, msg, file, QSL("%1").arg(line));
        break;
    }

    if (!lmsg.isEmpty()) {
        QString fmsg = QTime::currentTime().toString("h:mm:ss") + " " + lmsg;

        if (!cgl.isNull() && !cgl->logWindow.isNull()) {
            QMetaObject::invokeMethod(cgl->logWindow.data(), [fmsg](){
                    cgl->logWindow->updateMessages(fmsg);
                },Qt::QueuedConnection);
        }

        fmsg.append('\n');
        fprintf(stderr, "%s", fmsg.toLocal8Bit().constData());
    }

    loggerMutex.unlock();
}

CGlobal::CGlobal(QObject *parent) : QObject(parent)
{
    reg.reset(new CRegController(this));
    logWindow.reset(new CLogDisplay());

    loadSettings();
}

CGlobal::~CGlobal() = default;

bool CGlobal::safeToClose(int idx) const
{
    if (idx >= 0 && idx < reg->getHivesCount()) {
        if (!reg->saveTopHive(idx))
            return false;

        reg->closeTopHive(idx);
        return true;
    }

    writeSettings();

    for (int i = 0; i < reg->getHivesCount(); i++) {
        if (!reg->saveTopHive(i))
            return false;
    }

    while (reg->getHivesCount() > 0)
        reg->closeTopHive(0);

    return true;
}

void CGlobal::loadSettings()
{
    QSettings settings("kernel1024", "qregedit");
    settings.beginGroup("Main");
    hiveOpenMode = settings.value("hiveOpenMode", 0).toInt();
    settings.endGroup();
}

void CGlobal::writeSettings() const
{
    QSettings settings("kernel1024", "qregedit");
    settings.beginGroup("Main");
    settings.remove("");
    settings.setValue("hiveOpenMode", hiveOpenMode);
    settings.endGroup();
}

void CGlobal::settingsDialog(QWidget *parent)
{
    auto *dlg = new CSettingsDlg(parent);
    dlg->ui->checkNoAlloc->setChecked((hiveOpenMode & HMODE_NOALLOC) != 0);
    dlg->ui->checkNoExpand->setChecked((hiveOpenMode & HMODE_NOEXPAND) != 0);

    if (dlg->exec() == QDialog::Accepted) {
        if (dlg->ui->checkNoExpand->isChecked()) {
            hiveOpenMode |= HMODE_NOEXPAND;
        } else {
            hiveOpenMode &= ~HMODE_NOEXPAND;
        }

        if (dlg->ui->checkNoAlloc->isChecked()) {
            hiveOpenMode |= HMODE_NOALLOC;
        } else {
            hiveOpenMode &= ~HMODE_NOALLOC;
        }
    }

    dlg->deleteLater();
}

QString getOpenFileNameD ( QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
}

QStringList getOpenFileNamesD ( QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options )
{
    return QFileDialog::getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);
}

QStringList getSuffixesFromFilter(const QString &filter)
{
    QStringList res;
    res.clear();

    if (filter.isEmpty()) return res;

    res = filter.split(";;", Qt::SkipEmptyParts);

    if (res.isEmpty()) return res;

    QString ex = res.first();
    res.clear();

    if (ex.isEmpty()) return res;

    ex.remove(QRegularExpression("^.*\\("));
    ex.remove(QRegularExpression("\\).*$"));
    ex.remove(QRegularExpression("^.*\\."));
    res = ex.split(" ");

    return res;
}

QString getSaveFileNameD ( QWidget *parent, const QString &caption, const QString &dir,
                         const QString &filter, QString *selectedFilter, QFileDialog::Options options,
                         const QString &preselectFileName )
{
    QFileDialog d(parent, caption, dir, filter);
    d.setFileMode(QFileDialog::AnyFile);
    d.setOptions(options);
    d.setAcceptMode(QFileDialog::AcceptSave);

    QStringList sl;

    if (selectedFilter != nullptr && !selectedFilter->isEmpty()) {
        sl = getSuffixesFromFilter(*selectedFilter);
    } else {
        sl = getSuffixesFromFilter(filter);
    }

    if (!sl.isEmpty())
        d.setDefaultSuffix(sl.first());

    if (selectedFilter && !selectedFilter->isEmpty())
        d.selectNameFilter(*selectedFilter);

    if (!preselectFileName.isEmpty())
        d.selectFile(preselectFileName);

    if (d.exec() == QDialog::Accepted) {
        if (selectedFilter != nullptr)
            *selectedFilter = d.selectedNameFilter();

        if (!d.selectedFiles().isEmpty())
            return d.selectedFiles().first();
    }
    return QString();
}

QString	getExistingDirectoryD ( QWidget *parent, const QString &caption, const QString &dir, QFileDialog::Options options )
{
    return QFileDialog::getExistingDirectory(parent, caption, dir, options);
}
