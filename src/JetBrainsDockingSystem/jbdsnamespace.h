#ifndef JBDSNAMESPACE_H
#define JBDSNAMESPACE_H

#include <QtCore/QObject>

#include <JetBrainsDockingSystem/jbdsglobal.h>

namespace JBDS {

    Q_NAMESPACE_EXPORT(JBDS_EXPORT)

    enum ViewMode {
        DockPinned,
        Floating,
        Window,
    };
    Q_ENUM_NS(ViewMode)

    enum Side {
        Front,
        Back,
    };
    Q_ENUM_NS(Side)

    enum Orientation {
        Horizontal,
        TopToBottom,
        BottomToTop,
    };
    Q_ENUM_NS(Orientation)

}

#endif // JBDSNAMESPACE_H
