#include "MainWindow.h"

#include <QDebug>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    dock = new JBDS::DockWidget();
    dock->setDockAttribute(JBDS::DockWidget::ViewModeContextMenu, true);
    setCentralWidget(dock);

    dock->addWidget(Qt::LeftEdge, JBDS::Front, new QLabel("123"))->setText("123");
    dock->addWidget(Qt::LeftEdge, JBDS::Front, new QLabel("456"))->setText("456");
    dock->addWidget(Qt::RightEdge, JBDS::Front, new QLabel("789"))->setText("789");

    resize(1280, 720);

    setStyleSheet(R"(

MainWindow {
    background-color: #ffffff;
}




JBDS--DockWidget {
    background-color: transparent;
}

JBDS--DockWidget>JBDS--DockSideBar {
    padding: 3px;
    qproperty-buttonSpacing: 6;
}

JBDS--DockWidget>JBDS--DockSideBar[highlight=false] {
    background-color: transparent;
}

JBDS--DockWidget>JBDS--DockSideBar[highlight=true] {
    background-color: #f3f3f3;
}

JBDS--DockWidget QSplitter#dock-splitter:handle {
    background-color: red;
}

JBDS--DockWidget JBDS--DockPanel:handle {
    background-color: red;
}

    )");
}

MainWindow::~MainWindow() {
}
