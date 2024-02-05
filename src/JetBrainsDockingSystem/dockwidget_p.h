#ifndef DOCKWIDGET_P_H
#define DOCKWIDGET_P_H

#include <JetBrainsDockingSystem/dockwidget.h>

namespace JBDS {

    class DockWidgetPrivate {
        Q_DECLARE_PUBLIC(DockWidget)
    public:
        DockWidgetPrivate();
        virtual ~DockWidgetPrivate();

        void init();

        DockWidget *q_ptr;

        QScopedPointer<DockButtonDelegate> delegate;
    };

}

#endif // DOCKWIDGET_P_H
