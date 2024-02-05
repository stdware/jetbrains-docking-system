#include "dockwidget.h"
#include "dockwidget_p.h"

#include "dockbutton.h"

namespace JBDS {

    DockWidgetPrivate::DockWidgetPrivate() {
    }

    DockWidgetPrivate::~DockWidgetPrivate() {
    }

    void DockWidgetPrivate::init() {
    }

    DockWidget::DockWidget(QWidget *parent)
        : DockWidget(*new DockWidgetPrivate(), new DefaultDockButtonDelegate(), parent) {
    }

    DockWidget::DockWidget(DockButtonDelegate *delegate, QWidget *parent)
        : DockWidget(*new DockWidgetPrivate(), delegate, parent) {
    }

    DockWidget::~DockWidget() {
    }

    DockWidget::DockWidget(DockWidgetPrivate &d, DockButtonDelegate *delegate, QWidget *parent)
        : QFrame(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.delegate.reset(delegate);

        d.init();
    }

}