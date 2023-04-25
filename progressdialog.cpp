#include "progressdialog.h"
#include "ui_progressdialog.h"

CProgressDialog::CProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CProgressDialog)
{
    ui->setupUi(this);

    ui->label->setText(tr("Searching registry"));
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);

    setWindowTitle(tr("Registry Editor search"));
    setWindowModality(Qt::WindowModal);

    connect(ui->buttonCancel,&QPushButton::clicked,[this](){
        m_canceled = true;
        Q_EMIT cancel();
    });
}

CProgressDialog::~CProgressDialog()
{
    delete ui;
}

bool CProgressDialog::wasCanceled()
{
    const bool c = m_canceled;
    m_canceled = false;
    return c;
}

void CProgressDialog::setLabelText(const QString &text)
{
    ui->label->setText(text);
}

void CProgressDialog::setMinimum(int min)
{
    ui->progressBar->setMinimum(min);
}

void CProgressDialog::setMaximum(int max)
{
    ui->progressBar->setMaximum(max);
}

void CProgressDialog::setValue(int value)
{
    ui->progressBar->setValue(value);
}
