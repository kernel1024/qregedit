#include <QScrollBar>
#include <QDebug>
#include <QInputDialog>
#include <QMenu>
#include "logdisplay.h"
#include "global.h"
#include "ui_logdisplay.h"

CLogDisplay::CLogDisplay(QWidget *parent) :
    QDialog(parent, Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint),
    ui(new Ui::CLogDisplay)
{
    ui->setupUi(this);
    syntax = new CSpecLogHighlighter(ui->logView->document());

    updateMessages(QString());
}

CLogDisplay::~CLogDisplay()
{
    delete ui;
}

void CLogDisplay::updateMessages(const QString &message)
{
    if (!message.isEmpty())
        debugMessages.append(message);

    while (debugMessages.count() > 5000)
        debugMessages.removeFirst();

    if (!isVisible()) return;

    int sv = -1;

    if (ui->logView->verticalScrollBar() != nullptr)
        sv = ui->logView->verticalScrollBar()->value();

    updateText(debugMessages.join('\n'));

    if (ui->logView->verticalScrollBar() != nullptr) {
        if (!ui->checkScrollLock->isChecked()) {
            ui->logView->verticalScrollBar()->setValue(ui->logView->verticalScrollBar()->maximum());
        } else if (sv != -1) {
            ui->logView->verticalScrollBar()->setValue(sv);
        }
    }
}

void CLogDisplay::updateText(const QString &text)
{
    ui->logView->setPlainText(text);
}

void CLogDisplay::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    updateMessages(QString());

    if (firstShow && QApplication::activeWindow() != nullptr) {
        QPoint p = QApplication::activeWindow()->pos();
        p.rx() += 200;
        p.ry() += 100;
        move(p);
        firstShow = false;
    }
}

CSpecLogHighlighter::CSpecLogHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
}

void CSpecLogHighlighter::highlightBlock(const QString &text)
{
    formatBlock(text,QRegularExpression(QSL("^\\S{,8}"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::black,true);
    formatBlock(text,QRegularExpression(QSL("\\s(\\S+\\s)?Debug:\\s"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::black,true);
    formatBlock(text,QRegularExpression(QSL("\\s(\\S+\\s)?Warning:\\s"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::darkRed,true);
    formatBlock(text,QRegularExpression(QSL("\\s(\\S+\\s)?Critical:\\s"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::red,true);
    formatBlock(text,QRegularExpression(QSL("\\s(\\S+\\s)?Fatal:\\s"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::red,true);
    formatBlock(text,QRegularExpression(QSL("\\s(\\S+\\s)?Info:\\s"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::darkBlue,true);
    formatBlock(text,QRegularExpression(QSL("\\(\\S+\\)$"),
                                         QRegularExpression::CaseInsensitiveOption),Qt::gray,false,true);
}

void CSpecLogHighlighter::formatBlock(const QString &text,
                                      const QRegularExpression &exp,
                                      const QColor &color,
                                      bool weight,
                                      bool italic,
                                      bool underline,
                                      bool strikeout)
{
    if (text.isEmpty()) return;

    QTextCharFormat fmt;
    fmt.setForeground(color);
    if (weight) {
        fmt.setFontWeight(QFont::Bold);
    } else {
        fmt.setFontWeight(QFont::Normal);
    }
    fmt.setFontItalic(italic);
    fmt.setFontUnderline(underline);
    fmt.setFontStrikeOut(strikeout);

    auto it = exp.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), fmt);
    }
}
