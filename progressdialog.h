#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class CProgressDialog;
}

class CProgressDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(CProgressDialog)

private:
    Ui::CProgressDialog *ui;
    bool m_canceled { false };

public:
    explicit CProgressDialog(QWidget *parent = nullptr);
    ~CProgressDialog() override;

    bool wasCanceled();
    void setLabelText(const QString& text);

    void setMinimum(int min);
    void setMaximum(int max);
    void setValue(int value);

Q_SIGNALS:
    void cancel();

};

#endif // PROGRESSDIALOG_H
