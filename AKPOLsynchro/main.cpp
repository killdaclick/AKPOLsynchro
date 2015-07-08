#include "MainWindow.h"
#include <QApplication>
#include <QString>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.hide();

    return a.exec();
}
