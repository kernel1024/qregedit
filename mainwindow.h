#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
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

public slots:
    void openHive();
    void showValues(const QModelIndex& key);

};

#endif // MAINWINDOW_H
