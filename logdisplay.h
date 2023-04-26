#ifndef LOGDISPLAY_H
#define LOGDISPLAY_H

#include <QDialog>
#include <QStringList>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

namespace Ui {
class CLogDisplay;
}

class CLogDisplay : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(CLogDisplay)

public:
    explicit CLogDisplay(QWidget *parent = nullptr);
    ~CLogDisplay() override;

public Q_SLOTS:
    void updateMessages(const QString &message);

private:
    Ui::CLogDisplay *ui;
    bool firstShow { true };
    QSyntaxHighlighter *syntax { nullptr };
    QStringList debugMessages;

    void updateText(const QString &text);

protected:
    void showEvent(QShowEvent *event) override;
};

class CSpecLogHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit CSpecLogHighlighter(QTextDocument *parent);
protected:
    void highlightBlock(const QString &text) override;
private:
    void formatBlock(const QString &text,
                     const QRegularExpression &exp,
                     const QColor &color = Qt::black,
                     bool weight = false,
                     bool italic = false,
                     bool underline = false,
                     bool strikeout = false);
};

#endif // LOGDISPLAY_H
