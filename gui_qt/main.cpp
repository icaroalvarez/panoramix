#include "mainwindow.h"
#include <QApplication>
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    w.setWindowState(Qt::WindowMaximized);
    //w.start();

    return a.exec();
}