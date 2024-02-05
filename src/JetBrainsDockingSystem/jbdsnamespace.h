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

    enum Side {
        Front,
        Back,
    };

}

#endif // JBDSNAMESPACE_H
