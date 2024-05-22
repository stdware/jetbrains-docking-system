// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#ifndef DOCKBUTTONDELEGATE_H
#define DOCKBUTTONDELEGATE_H

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QMenu>

#include <JetBrainsDockingSystem/jbdsnamespace.h>

namespace JBDS {

    class JBDS_EXPORT DockButtonDelegate {
    public:
        virtual ~DockButtonDelegate();

        virtual QAbstractButton *create(QWidget *parent) const = 0;

        virtual Orientation buttonOrientation(const QAbstractButton *button) const = 0;
        virtual void setButtonOrientation(QAbstractButton *button,
                                          Orientation orientation) const = 0;

        virtual QMenu *createViewModeMenu(QAbstractButton *button, QWidget *parent) const = 0;

        inline bool buttonDockVisible(const QAbstractButton *button) const;
    };

}

#endif // DOCKBUTTONDELEGATE_H
