#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QCloseEvent>
#include <QSortFilterProxyModel>
#include <QPointer>
#include "registrymodel.h"
#include "sammodel.h"
#include "progressdialog.h"

namespace Ui {
class MainWindow;
}

class CMainWindow : public QMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(CMainWindow)

public:
    explicit CMainWindow(QWidget *parent = nullptr);
    ~CMainWindow() override;

    void centerWindow();

private:
    Ui::MainWindow *ui;

    QPointer<CRegistryModel> treeModel;
    QPointer<CValuesModel> valuesModel;
    QPointer<CSAMGroupsModel> groupsModel;
    QPointer<CSAMUsersModel> usersModel;
    QPointer<QSortFilterProxyModel> valuesSortModel;
    QPointer<CProgressDialog> searchProgressDialog;

    void treeCtxMenuPrivate(const QPoint& pos, bool fromValuesTable);

protected:
    void closeEvent(QCloseEvent *event) override;

Q_SIGNALS:
    void startSearch(const QModelIndex &idx, const QString &text);

public Q_SLOTS:
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
    void editUser(const QModelIndex& index);

};

#endif // MAINWINDOW_H
