#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // 解决高分屏模糊问题
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
