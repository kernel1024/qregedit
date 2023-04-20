#ifndef LOGDISPLAY_H
#define LOGDISPLAY_H

#include <QDialog>
#include <QStringList>
#include <QSyntaxHighlighter>
#include <QRegExp>

namespace Ui {
    class CLogDisplay;
}

class CLogDisplay : public QDialog {
    Q_OBJECT

  public:
    explicit CLogDisplay();
    ~CLogDisplay();

  public slots:
    void updateMessages();

  private:
    Ui::CLogDisplay *ui;
    QStringList savedMessages;
    bool firstShow;
    QSyntaxHighlighter *syntax;

    void updateText(const QString &text);

  protected:
    void showEvent(QShowEvent *);
};

class CSpecLogHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
  public:
    CSpecLogHighlighter(QTextDocument *parent);
  protected:
    void highlightBlock(const QString &text);
  private:
    void formatBlock(const QString &text,
                     const QRegExp &exp,
                     const QColor &color = Qt::black,
                     bool weight = false,
                     bool italic = false,
                     bool underline = false,
                     bool strikeout = false);
};

#endif // LOGDISPLAY_H
