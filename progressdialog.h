#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class CProgressDialog;
}

class CProgressDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::CProgressDialog *ui;
    bool m_canceled;

public:
    explicit CProgressDialog(QWidget *parent = 0);
    ~CProgressDialog();

    bool wasCanceled();
    void setLabelText(const QString& text);

    void setMinimum(int min);
    void setMaximum(int max);
    void setValue(int value);

};

#endif // PROGRESSDIALOG_H
