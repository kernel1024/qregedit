#include "mainwindow.h"
#include "global.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    debugMessages.clear();
    qInstallMessageHandler(stdConsoleOutput);

    QApplication a(argc, argv);
    CMainWindow w;
    w.show();

    return a.exec();
}
