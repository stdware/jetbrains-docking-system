// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#ifndef DOCKSIDEBAR_P_H
#define DOCKSIDEBAR_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the JetBrainsDockingSystem API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtWidgets/QBoxLayout>

#include <JetBrainsDockingSystem/dockwidget.h>

namespace JBDS {

    class DockSideBar : public QFrame {
        Q_OBJECT
        Q_PROPERTY(int buttonSpacing READ buttonSpacing WRITE setButtonSpacing)
    public:
        explicit DockSideBar(DockWidget *dock, Qt::Edge edge, QWidget *parent = nullptr);
        ~DockSideBar();

        QSize sizeHint() const override;

        inline DockWidget *dock() const {
            return m_dock;
        }

        inline Qt::Edge edge() const {
            return m_edge;
        }

        inline Qt::Orientation orientation() const {
            return (m_buttonOrientation == Horizontal) ? Qt::Horizontal : Qt::Vertical;
        }

        inline QBoxLayout *buttonLayout(Side side) const {
            return (side == Front) ? m_firstLayout : m_secondLayout;
        }

        void insertButton(Side side, int index, QAbstractButton *button);
        void removeButton(Side side, QAbstractButton *button);

        inline QList<QAbstractButton *> buttons(Side side) const {
            return (side == Front) ? m_firstCards : m_secondCards;
        }

        inline int count(Side side) const {
            return ((side == Front) ? m_firstCards : m_secondCards).size();
        }

        void buttonToggled(Side side, QAbstractButton *button);
        void buttonViewModeChanged(Side side, QAbstractButton *button);

        bool highlight() const;
        void setHighlight(bool highlight, int widthHint = 0);

        int buttonSpacing() const;
        void setButtonSpacing(int spacing);

    protected:
        DockWidget *m_dock;

        Qt::Edge m_edge;
        Orientation m_buttonOrientation;

        QBoxLayout *m_layout;
        QBoxLayout *m_firstLayout;
        QBoxLayout *m_secondLayout;

        QList<QAbstractButton *> m_firstCards;
        QList<QAbstractButton *> m_secondCards;

        int m_widthHint;
    };

}

#endif // DOCKSIDEBAR_P_H
