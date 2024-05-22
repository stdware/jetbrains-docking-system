// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#include "dockpanel_p.h"

namespace JBDS {

    DockPanel::DockPanel(Qt::Orientation orient, QWidget *parent) : QSplitter(orient, parent) {
        setChildrenCollapsible(false);

        m_firstWidget = new QStackedWidget();
        m_secondWidget = new QStackedWidget();

        QSplitter::addWidget(m_firstWidget);
        QSplitter::addWidget(m_secondWidget);

        m_firstWidget->hide();
        m_secondWidget->hide();
        updateVisibility();
    }

    DockPanel::~DockPanel() {
    }

    int DockPanel::addWidget(Side side, QWidget *w, bool visible) {
        return insertWidget(
            side, (side == Front) ? m_firstWidget->count() : m_secondWidget->count(), w, visible);
    }

    int DockPanel::insertWidget(Side side, int index, QWidget *w, bool visible) {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        int res = container->insertWidget(index, w);
        if (visible) {
            container->show();
            container->setCurrentWidget(w);
            if (!isVisible()) {
                show();
            }
        }
        return res;
    }

    void DockPanel::removeWidget(Side side, QWidget *w) {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        bool isCurrent = container->currentWidget() == w;
        container->removeWidget(w);

        if (isCurrent && container->isVisible()) {
            container->hide();
            updateVisibility();
        }
    }

    QWidget *DockPanel::currentWidget(Side side) const {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->currentWidget();
    }

    int DockPanel::currentIndex(Side side) const {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->currentIndex();
    }

    int DockPanel::indexOf(Side side, QWidget *w) const {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->indexOf(w);
    }

    QWidget *DockPanel::widget(Side side, int index) const {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->widget(index);
    }

    int DockPanel::count(Side side) const {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->count();
    }

    void DockPanel::setCurrentIndex(Side side, int index) {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->setCurrentIndex(index);
    }

    void DockPanel::setCurrentWidget(Side side, QWidget *w) {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        return container->setCurrentWidget(w);
    }

    void DockPanel::setContainerVisible(Side side, bool visible) {
        auto container = (side == Front) ? m_firstWidget : m_secondWidget;
        container->setVisible(visible);
        if (visible) {
            show();
        } else {
            updateVisibility();
        }
    }

    void DockPanel::updateVisibility() {
        if (!m_firstWidget->isVisible() && !m_secondWidget->isVisible()) {
            hide();
        }
    }

}
