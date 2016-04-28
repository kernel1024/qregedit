#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QCloseEvent>
#include <QSortFilterProxyModel>
#include "registrymodel.h"
#include "sammodel.h"
#include "progressdialog.h"

namespace Ui {
class MainWindow;
}

class CMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CMainWindow(QWidget *parent = 0);
    ~CMainWindow();

    void centerWindow();
private:
    Ui::MainWindow *ui;
    CRegistryModel *treeModel;
    CValuesModel *valuesModel;
    CSAMGroupsModel *groupsModel;
    CSAMUsersModel *usersModel;
    QSortFilterProxyModel *valuesSortModel;
    CProgressDialog *searchProgressDialog;
    void treeCtxMenuPrivate(const QPoint& pos, const bool fromValuesTable);

protected:
    void closeEvent(QCloseEvent *event);

signals:
    void startSearch(const QModelIndex &idx, const QString &text);

public slots:
    void openHive();
    void importReg();
    void showValues(const QModelIndex& key);
    void hivePrepareClose(int idx);
    void treeCtxMenu(const QPoint& pos) { treeCtxMenuPrivate(pos, false); }
    void valuesCtxMenu(const QPoint& pos);
    void valuesModify(const QModelIndex& key);
    void createEntry();
    void about();
    void keyFound(const QModelIndex& key, const QString& value);
    void searchTxt();
    void searchFinished();
    void deleteValue(const QModelIndex& value);

};

#endif // MAINWINDOW_H
