#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QtWidgets/QFrame>

#include <JetBrainsDockingSystem/dockbuttondelegate.h>

namespace JBDS {

    class DockWidgetPrivate;

    class DockWidget : public QFrame {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DockWidget)
    public:
        DockWidget(QWidget *parent = nullptr);
        DockWidget(DockButtonDelegate *delegate, QWidget *parent = nullptr);
        ~DockWidget();

    protected:
        DockWidget(DockWidgetPrivate &d, DockButtonDelegate *delegate, QWidget *parent = nullptr);

        QScopedPointer<DockWidgetPrivate> d_ptr;
    };

}

#endif // DOCKWIDGET_H
