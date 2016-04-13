#include <QFileDialog>
#include <QDesktopWidget>
#include <QIcon>
#include <QMessageBox>
#include <QInputDialog>

#include "global.h"
#include "mainwindow.h"
#include "valueeditor.h"
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

    valuesSortModel = new QSortFilterProxyModel(this);
    valuesSortModel->setSourceModel(valuesModel);
    ui->treeHives->setModel(treeModel);
    ui->tableValues->setModel(valuesSortModel);

    ui->treeHives->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableValues->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->actionExit,&QAction::triggered,this,&CMainWindow::close);
    connect(ui->actionOpenHive,&QAction::triggered,this,&CMainWindow::openHive);

    connect(ui->treeHives,&QTreeView::clicked,this,&CMainWindow::showValues);
    connect(ui->treeHives,&QTreeView::customContextMenuRequested,
            this,&CMainWindow::treeCtxMenu);

    connect(ui->tableValues,&QTableView::customContextMenuRequested,
            this,&CMainWindow::valuesCtxMenu);
    connect(ui->tableValues,&QTableView::activated,
            this,&CMainWindow::valuesModify);

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

void CMainWindow::treeCtxMenuPrivate(const QPoint &pos, const bool fromValuesTable)
{
    QModelIndex idx;
    QMenu* cm;
    if (fromValuesTable) {
        idx = ui->treeHives->currentIndex();
        cm = new QMenu(ui->tableValues);
    } else {
        idx = ui->treeHives->indexAt(pos);
        cm = new QMenu(ui->treeHives);
    }

    int hive = treeModel->getHiveIdx(idx);
    if (hive<0) return;

    QAction* acm;
    // TODO: add key operations...
    acm = cm->addAction(tr("Close hive"));
    connect(acm,&QAction::triggered,[hive](){
        cgl->reg->closeTopHive(hive);
    });

    if (fromValuesTable)
        cm->exec(ui->tableValues->mapToGlobal(pos));
    else
        cm->exec(ui->treeHives->mapToGlobal(pos));
    cm->deleteLater();
}

void CMainWindow::valuesCtxMenu(const QPoint &pos)
{
    QModelIndex uidx = ui->tableValues->indexAt(pos);
    QModelIndex idx = valuesSortModel->mapToSource(uidx);

    QString name = valuesModel->getValueName(idx);

    QMenu cm(ui->tableValues);
    QAction* acm;
    if (idx.isValid()) { // Context menu for value
        acm = cm.addAction(tr("Modify"));
        connect(acm,&QAction::triggered,[this,uidx](){
            valuesModify(uidx);
        });

        cm.addSeparator();

        acm = cm.addAction(tr("Delete"));
        acm->setDisabled(valuesModel->getValue(idx).isDefault());
        connect(acm,&QAction::triggered,[this,idx,name](){
            if (!valuesModel->deleteValue(idx))
                QMessageBox::critical(this,tr("Registry Editor"),tr("Failed to delete value '%1'.")
                                      .arg(name));
        });

        acm = cm.addAction(tr("Rename"));
        acm->setEnabled(false); // temporary
        connect(acm,&QAction::triggered,[this,name,idx](){
            bool ok;
            QString name = QInputDialog::getText(this,tr("Registry Editor"),
                                                     tr("Rename registry value"),QLineEdit::Normal,name,&ok);
            if (ok && !name.isEmpty())
                valuesModel->renameValue(idx,name);
        });

        cm.exec(ui->tableValues->mapToGlobal(pos));
    } else
        treeCtxMenuPrivate(pos, true);
}

void CMainWindow::valuesModify(const QModelIndex &key)
{
    QModelIndex idx = valuesSortModel->mapToSource(key);

    CValueEditor* dlg = new CValueEditor(this,idx);
    if (!dlg->initFailed())
        dlg->exec();
    dlg->deleteLater();
}
