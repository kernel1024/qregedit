#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    CRegistryModel *model;

public slots:
    void openHive();

};

#endif // MAINWINDOW_H
