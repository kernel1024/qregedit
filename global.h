#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include <QFileDialog>
#include "regutils.h"

class CGlobal : public QObject
{
    Q_OBJECT
public:
    int hiveOpenMode;
    CRegController* reg;

    explicit CGlobal(QObject *parent = 0);

    bool safeToClose(int idx = -1);

    void loadSettings();
    void writeSettings();

    void settingsDialog(QWidget *parent = NULL);

signals:

public slots:
};

extern CGlobal* cgl;

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
