#include <QFileDialog>
#include <QDesktopWidget>
#include <QSortFilterProxyModel>
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

    treeModel = new CRegistryModel();
    valuesModel = new CValuesModel();

    QSortFilterProxyModel *sortValues = new QSortFilterProxyModel(this);
    sortValues->setSourceModel(valuesModel);
    ui->treeHives->setModel(treeModel);
    ui->tableValues->setModel(sortValues);

    ui->treeHives->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->actionExit,&QAction::triggered,this,&CMainWindow::close);
    connect(ui->actionOpenHive,&QAction::triggered,this,&CMainWindow::openHive);

    connect(ui->treeHives,&QTreeView::clicked,this,&CMainWindow::showValues);
    connect(ui->treeHives,&QTreeView::customContextMenuRequested,
            this,&CMainWindow::treeCtxMenu);
    connect(cgl->reg,&CRegController::hiveAboutToClose,this,&CMainWindow::hivePrepareClose);

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

    QList<int> sz;
    sz << nw.width()/4 << 3*nw.width()/4;
    ui->splitter->setSizes(sz);
}

void CMainWindow::closeEvent(QCloseEvent *event)
{
    if (cgl!=NULL && cgl->safeToClose())
        event->accept();
    else
        event->ignore();
}

void CMainWindow::openHive()
{
    QString fname = QFileDialog::getOpenFileName(this,tr("Registry files"));
    if (!fname.isEmpty())
        if (!cgl->reg->openTopHive(fname))
            QMessageBox::critical(this,tr("Registry Editor"),tr("Failed to open hive file.\n"
                                                                "See standard output for debug messages."));
}

void CMainWindow::showValues(const QModelIndex &key)
{
    valuesModel->keyChanged(key, ui->tableValues);
}

void CMainWindow::hivePrepareClose(int idx)
{
    Q_UNUSED(idx)

    valuesModel->keyChanged(QModelIndex(),ui->tableValues);
}

void CMainWindow::treeCtxMenu(const QPoint &pos)
{
    QModelIndex idx = ui->treeHives->indexAt(pos);

    int hive = treeModel->getHiveIdx(idx);
    if (hive<0) return;

    QMenu cm(ui->treeHives);

    QAction* acm;
    acm = cm.addAction(tr("Close hive"));
    connect(acm,&QAction::triggered,[hive](){
        cgl->reg->closeTopHive(hive);
    });
    cm.exec(ui->treeHives->mapToGlobal(pos));
}
