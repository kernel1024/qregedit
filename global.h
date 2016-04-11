#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include "regutils.h"

class CGlobal : public QObject
{
    Q_OBJECT
public:
    int hiveOpenMode;
    CRegController* reg;

    explicit CGlobal(QObject *parent = 0);

    bool safeToClose();

    void loadSettings();
    void writeSettings();

signals:

public slots:
};

extern CGlobal* cgl;

#endif // GLOBAL_H
