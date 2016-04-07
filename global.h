#ifndef GLOBAL_H
#define GLOBAL_H

#include <QObject>
#include "regutils.h"

class CGlobal : public QObject
{
    Q_OBJECT
public:
    int hiveMode;
    CRegController* reg;

    explicit CGlobal(QObject *parent = 0);

signals:

public slots:
};

extern CGlobal* cgl;

#endif // GLOBAL_H
