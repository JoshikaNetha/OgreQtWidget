#include "mainwindow.h"

#include <QApplication>
// #include "OgreDemo.h"
// #include "OgreWidget.h"
#include <QDebug>
int main(int argc, char *argv[])
{
    qDebug()<<"Started";
//     QApplication a(argc, argv);
//     qDebug()<<"Started ogreWidget";

//     QMainWindow mainWindow;
//     OgreWidget* ogreWidget = new OgreWidget();

//     mainWindow.setCentralWidget(ogreWidget);
//     mainWindow.resize(800, 600);
//     mainWindow.show();

//     return a.exec();
    QApplication a(argc, argv);
    MainWindow w;
    w.resize(800, 600);
    w.show();
    return a.exec();
}
