#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include <QFileDialog>
#include <QStringList>
#include "logdisplay.h"
#include "regutils.h"

extern QStringList debugMessages;

class CGlobal : public QObject
{
    Q_OBJECT
public:
    int hiveOpenMode;
    CRegController* reg;
    CLogDisplay* logWindow;

    explicit CGlobal(QObject *parent = 0);
    ~CGlobal();

    bool safeToClose(int idx = -1);

    void loadSettings();
    void writeSettings();

    void settingsDialog(QWidget *parent = NULL);

signals:

public slots:
};

extern CGlobal* cgl;

void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

QString getOpenFileNameD ( QWidget * parent = 0, const QString & caption = QString(),
                           const QString & dir = QString(), const QString & filter = QString(),
                           QString * selectedFilter = 0, QFileDialog::Options options = 0);

QStringList getOpenFileNamesD ( QWidget * parent = 0, const QString & caption = QString(),
                               const QString & dir = QString(), const QString & filter = QString(),
                               QString * selectedFilter = 0, QFileDialog::Options options = 0);

QString getSaveFileNameD (QWidget * parent = 0, const QString & caption = QString(),
                          const QString & dir = QString(), const QString & filter = QString(),
                          QString * selectedFilter = 0, QFileDialog::Options options = 0,
                          QString preselectFileName = QString());

QString	getExistingDirectoryD ( QWidget * parent = 0, const QString & caption = QString(),
                                const QString & dir = QString(),
                                QFileDialog::Options options = QFileDialog::ShowDirsOnly);

#endif // GLOBAL_H
