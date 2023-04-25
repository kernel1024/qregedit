#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include <QFileDialog>
#include <QStringList>
#include <QScopedPointer>
#include "logdisplay.h"
#include "regutils.h"

#define QSL QStringLiteral

extern QStringList debugMessages;

class CGlobal : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(CGlobal)

public:
    int hiveOpenMode { 0 };
    QScopedPointer<CRegController> reg;
    QScopedPointer<CLogDisplay> logWindow;

    explicit CGlobal(QObject *parent = nullptr);
    ~CGlobal() override;

    bool safeToClose(int idx = -1) const;

    void loadSettings();
    void writeSettings() const;

    void settingsDialog(QWidget *parent = nullptr);
};

extern CGlobal *cgl;

void stdConsoleOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

QString getOpenFileNameD ( QWidget *parent = nullptr, const QString &caption = QString(),
                         const QString &dir = QString(), const QString &filter = QString(),
                         QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());

QStringList getOpenFileNamesD ( QWidget *parent = nullptr, const QString &caption = QString(),
                              const QString &dir = QString(), const QString &filter = QString(),
                              QString *selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());

QString getSaveFileNameD(QWidget *parent = nullptr,
                         const QString &caption = QString(),
                         const QString &dir = QString(),
                         const QString &filter = QString(),
                         QString *selectedFilter = nullptr,
                         QFileDialog::Options options = QFileDialog::Options(),
                         const QString &preselectFileName = QString());

QString	getExistingDirectoryD ( QWidget *parent = nullptr, const QString &caption = QString(),
                              const QString &dir = QString(),
                              QFileDialog::Options options = QFileDialog::ShowDirsOnly);

#endif // GLOBAL_H
