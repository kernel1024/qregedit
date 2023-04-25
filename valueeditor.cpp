#include <QWidget>
#include <QMessageBox>
#include "global.h"
#include "registrymodel.h"
#include "valueeditor.h"
#include "ui_valueeditor.h"

CValueEditor::CValueEditor(QWidget *parent, int createType, const QModelIndex& idx) :
    QDialog(parent),
    ui(new Ui::CValueEditor)
{
    ui->setupUi(this);

    ui->spinDWORD->setMinimum(INT_MIN);
    ui->spinDWORD->setMaximum(INT_MAX);

    // replace widget placeholder with hex editor
    ui->page_hex->layout()->removeWidget(ui->widgetHex);
    ui->widgetHex->setParent(nullptr);
    delete ui->widgetHex;

    hexEditor = new QHexEdit(this);
    hexEditor->setObjectName(QString("hexEditor"));
    ui->page_hex->layout()->addWidget(hexEditor);
    hexEditor->setOverwriteMode(false);

    // various event handlers
    connect(ui->radioDWORD10,&QRadioButton::toggled,this,[this](bool checked){
        if (checked) ui->spinDWORD->setDisplayIntegerBase(10);
    });
    connect(ui->radioDWORD16,&QRadioButton::toggled,this,[this](bool checked){
        if (checked) ui->spinDWORD->setDisplayIntegerBase(16);
    });

    if (cgl==nullptr || !cgl->reg->valuesModel) return;

    m_createType = createType;
    valueIndex = idx;
    if (m_createType!=REG_NONE) {
        m_value = CValue(m_createType);
    } else {
        m_value = cgl->reg->valuesModel->getValue(valueIndex);
        if (m_value.isEmpty()) return;
    }
    ui->editValueName->setReadOnly(m_createType==REG_NONE);

    prepareWidgets();

    connect(ui->btnOk,&QPushButton::clicked,this,&CValueEditor::saveValue);

    m_initFailure = false;
}

CValueEditor::~CValueEditor()
{
    delete ui;
}

void CValueEditor::saveValue()
{
    if ((!valueIndex.isValid() && (m_createType==REG_NONE))
        || cgl==nullptr || !cgl->reg->valuesModel) return;

    if (m_createType!=REG_NONE)
        m_value.name = ui->editValueName->text();

    switch (m_value.type) {
    case REG_DWORD:
        m_value.vDWORD = ui->spinDWORD->value();
        break;
    case REG_SZ:
    case REG_EXPAND_SZ:
        m_value.vString = ui->editString->text();
        break;
    case REG_MULTI_SZ:
        m_value.vString = ui->editMultiString->toPlainText();
        break;
    default:
        m_value.vOther = hexEditor->data();
        break;
    }

    if (m_createType==REG_NONE) {
        if (!cgl->reg->valuesModel->setValue(valueIndex,m_value)) {
            QMessageBox::critical(this,tr("Registry Editor - Error"),tr("Failed to change value '%1'.")
                                                                           .arg(m_value.name));
            return;
        }
    }

    accept();
}

void CValueEditor::prepareWidgets()
{
    ui->editValueName->setText(m_value.name);
    ui->labelType->setText(cgl->reg->getValueTypeStr(m_value.type));

    switch (m_value.type) {
    case REG_DWORD:
        ui->stack->setCurrentWidget(ui->page_dword);
        ui->spinDWORD->setValue(m_value.vDWORD);
        break;
    case REG_SZ:
    case REG_EXPAND_SZ:
        ui->stack->setCurrentWidget(ui->page_sz);
        ui->editString->setText(m_value.vString);
        break;
    case REG_MULTI_SZ:
        ui->stack->setCurrentWidget(ui->page_multisz);
        ui->editMultiString->setPlainText(m_value.vString);
        break;
    default:
        ui->stack->setCurrentWidget(ui->page_hex);
        hexEditor->setData(m_value.vOther);
        break;
    }
}
