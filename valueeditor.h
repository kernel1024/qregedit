#ifndef VALUEEDITOR_H
#define VALUEEDITOR_H

#include <QDialog>
#include <QModelIndex>
#include "regutils.h"
#include "qhexedit.h"

namespace Ui {
class CValueEditor;
}

class CValue;

class CValueEditor : public QDialog
{
    Q_OBJECT

public:
    explicit CValueEditor(QWidget *parent, int createType, const QModelIndex &idx);
    ~CValueEditor();

    bool initFailed() { return m_initFailure; }
    CValue getValue() { return m_value; }

public slots:
    void saveValue();

private:
    Ui::CValueEditor *ui;
    CValue m_value;
    QHexEdit *hexEditor;
    QModelIndex valueIndex;
    bool m_initFailure;
    int m_createType;

    void prepareWidgets();
};

#endif // VALUEEDITOR_H
