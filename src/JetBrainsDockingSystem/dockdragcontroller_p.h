// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#ifndef DOCKDRAGCONTROLLER_P_H
#define DOCKDRAGCONTROLLER_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the JetBrainsDockingSystem API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtWidgets/QLabel>

#include <JetBrainsDockingSystem/dockwidget.h>
#include <JetBrainsDockingSystem/docksidebar_p.h>

namespace JBDS {

    class DockDragController : public QObject {
    public:
        explicit DockDragController(DockWidget *dock, QObject *parent = nullptr);
        ~DockDragController();

    public:
        void startDrag(QAbstractButton *button, const QPoint &pos, const QPixmap &pixmap);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

        DockWidget *m_dock;
        QWidget *m_label;

    private:
        void tabDragMove();
        void tabDragOver();
    };

}

#endif // DOCKDRAGCONTROLLER_P_H
