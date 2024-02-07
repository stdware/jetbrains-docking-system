#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <JetBrainsDockingSystem/dockwidget.h>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    JBDS::DockWidget *dock;

Q_SIGNALS:
};

#endif // MAINWINDOW_H
