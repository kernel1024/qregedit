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
    Q_DISABLE_COPY(CValueEditor)

public:
    explicit CValueEditor(QWidget *parent, int createType, const QModelIndex &idx);
    ~CValueEditor() override;

    bool initFailed() const { return m_initFailure; }
    CValue getValue() const { return m_value; }

public Q_SLOTS:
    void saveValue();

private:
    Ui::CValueEditor *ui;
    CValue m_value;
    QHexEdit *hexEditor; // TODO: handle it
    QModelIndex valueIndex;
    bool m_initFailure { true };
    int m_createType { REG_NONE };

    void prepareWidgets();
};

#endif // VALUEEDITOR_H
