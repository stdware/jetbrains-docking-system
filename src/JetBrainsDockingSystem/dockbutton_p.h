// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#ifndef DOCKBUTTON_P_H
#define DOCKBUTTON_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the JetBrainsDockingSystem API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <JetBrainsDockingSystem/dockbutton.h>

namespace JBDS {

    class DockButtonPrivate {
        Q_DECLARE_PUBLIC(DockButton)
    public:
        DockButtonPrivate();
        virtual ~DockButtonPrivate();

        void init();

        DockButton *q_ptr;

        Orientation orientation = Horizontal;
    };

}

#endif // DOCKBUTTON_P_H
