#include <QDebug>
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return QApplication::exec();
}