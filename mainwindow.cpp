#include <QFileDialog>
#include <QDesktopWidget>
#include <QIcon>
#include <QMessageBox>

#include "global.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

CMainWindow::CMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if (cgl==NULL)
        cgl = new CGlobal(this);

    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/regedit"));

    model = new CRegistryModel();

    ui->treeHives->setModel(model);
//    ui->tableValues->setModel(model);

    connect(ui->actionExit,&QAction::triggered,this,&CMainWindow::close);
    connect(ui->actionOpenHive,&QAction::triggered,this,&CMainWindow::openHive);

    centerWindow();
}

CMainWindow::~CMainWindow()
{
    delete ui;
}

void CMainWindow::centerWindow()
{
    int screen = 0;
    QWidget *w = window();
    QDesktopWidget *desktop = QApplication::desktop();
    if (w) {
        screen = desktop->screenNumber(w);
    } else if (desktop->isVirtualDesktop()) {
        screen = desktop->screenNumber(QCursor::pos());
    } else {
        screen = desktop->screenNumber(this);
    }
    QRect rect(desktop->availableGeometry(screen));
        int h = 80*rect.height()/100;
    QSize nw(135*h/100,h);
    if (nw.width()<1000) nw.setWidth(80*rect.width()/100);
    resize(nw);
    move(rect.width()/2 - frameGeometry().width()/2,
         rect.height()/2 - frameGeometry().height()/2);
}

void CMainWindow::openHive()
{
    QString fname = QFileDialog::getOpenFileName(this,tr("Registry files"));
    if (!fname.isEmpty())
        if (!cgl->reg->openTopHive(fname))
            QMessageBox::critical(this,tr("Registry Editor"),tr("Failed to open hive file.\n"
                                                                "See standard output for debug messages."));
}
