#ifndef DOCKBUTTON_P_H
#define DOCKBUTTON_P_H

#include <JetBrainsDockingSystem/dockbutton.h>

namespace JBDS {

    class DockButtonPrivate {
        Q_DECLARE_PUBLIC(DockButton)
    public:
        DockButtonPrivate();
        virtual ~DockButtonPrivate();

        void init();

        DockButton *q_ptr;

        Qt::Orientation orientation = Qt::Horizontal;
    };

}

#endif // DOCKBUTTON_P_H
