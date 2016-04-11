#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QCloseEvent>
#include "registrymodel.h"

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

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void openHive();
    void showValues(const QModelIndex& key);
    void hivePrepareClose(int idx);
    void treeCtxMenu(const QPoint& pos);

};

#endif // MAINWINDOW_H
