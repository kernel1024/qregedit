#include "global.h"

CGlobal* cgl = NULL;

CGlobal::CGlobal(QObject *parent) : QObject(parent)
{
    hiveMode = 0;
    reg = new CRegController(this);
}
