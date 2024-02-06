#include "dockdragcontroller_p.h"

namespace JBDS {

    DockDragLabel::DockDragLabel(QWidget *parent) : QLabel(parent) {
    }

    DockDragLabel::~DockDragLabel() {
    }

    DockDragController::DockDragController(DockWidget *dock, QObject *parent) {
    }

    DockDragController::~DockDragController() {
    }

    void DockDragController::startDrag(QAbstractButton *button, const QPoint &pos,
                                       const QPixmap &pixmap) {
    }

    bool DockDragController::eventFilter(QObject *obj, QEvent *event) {
        return QObject::eventFilter(obj, event);
    }

    void DockDragController::tabDragMove() {
    }

    void DockDragController::tabDragOver() {
    }

}