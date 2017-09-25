#include <QScrollBar>
#include <QDebug>
#include <QInputDialog>
#include <QMenu>
#include "logdisplay.h"
#include "mainwindow.h"
#include "global.h"
#include "ui_logdisplay.h"

CLogDisplay::CLogDisplay() :
    QDialog(nullptr, Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint),
    ui(new Ui::CLogDisplay)
{
    ui->setupUi(this);
    firstShow = true;
    syntax = new CSpecLogHighlighter(ui->logView->document());

    updateMessages();
}

CLogDisplay::~CLogDisplay()
{
    savedMessages.clear();
    delete ui;
}

void CLogDisplay::updateMessages()
{
    if (!isVisible()) return;
    int fr = -1;
    int sv = -1;
    if (ui->logView->verticalScrollBar()!=nullptr)
        sv = ui->logView->verticalScrollBar()->value();

    if (!savedMessages.isEmpty())
        fr = debugMessages.lastIndexOf(savedMessages.last());
    if (fr>=0 && fr<debugMessages.count()) {
        for (int i=(fr+1);i<debugMessages.count();i++)
            savedMessages << debugMessages.at(i);
    } else
        savedMessages = debugMessages;

    updateText(savedMessages.join('\n'));
    if (ui->logView->verticalScrollBar()!=nullptr) {
        if (!ui->checkScrollLock->isChecked())
            ui->logView->verticalScrollBar()->setValue(ui->logView->verticalScrollBar()->maximum());
        else if (sv!=-1)
            ui->logView->verticalScrollBar()->setValue(sv);
    }
}

void CLogDisplay::updateText(const QString &text)
{
    ui->logView->setPlainText(text);
}

void CLogDisplay::showEvent(QShowEvent *)
{
    updateMessages();

    if (firstShow && QApplication::activeWindow()!=nullptr) {
        QPoint p = QApplication::activeWindow()->pos();
        p.rx()+=200;
        p.ry()+=100;
        move(p);
        firstShow=false;
    }
}

CSpecLogHighlighter::CSpecLogHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{

}

void CSpecLogHighlighter::highlightBlock(const QString &text)
{
    formatBlock(text,QRegExp("^\\S{,8}",Qt::CaseInsensitive),Qt::black,true);
    formatBlock(text,QRegExp("\\s(\\S+\\s)?Debug:\\s",Qt::CaseInsensitive),Qt::black,true);
    formatBlock(text,QRegExp("\\s(\\S+\\s)?Warning:\\s",Qt::CaseInsensitive),Qt::darkRed,true);
    formatBlock(text,QRegExp("\\s(\\S+\\s)?Critical:\\s",Qt::CaseInsensitive),Qt::red,true);
    formatBlock(text,QRegExp("\\s(\\S+\\s)?Fatal:\\s",Qt::CaseInsensitive),Qt::red,true);
    formatBlock(text,QRegExp("\\s(\\S+\\s)?Info:\\s",Qt::CaseInsensitive),Qt::darkBlue,true);
    formatBlock(text,QRegExp("\\(\\S+\\)$",Qt::CaseInsensitive),Qt::gray,false,true);
}

void CSpecLogHighlighter::formatBlock(const QString &text, const QRegExp &exp,
                                      const QColor &color,
                                      bool weight,
                                      bool italic,
                                      bool underline,
                                      bool strikeout)
{
    if (text.isEmpty()) return;

    QTextCharFormat fmt;
    fmt.setForeground(color);
    if (weight)
        fmt.setFontWeight(QFont::Bold);
    else
        fmt.setFontWeight(QFont::Normal);
    fmt.setFontItalic(italic);
    fmt.setFontUnderline(underline);
    fmt.setFontStrikeOut(strikeout);

    int pos = 0;
    while ((pos=exp.indexIn(text,pos)) != -1) {
        int length = exp.matchedLength();
        setFormat(pos, length, fmt);
        pos += length;
    }
}
