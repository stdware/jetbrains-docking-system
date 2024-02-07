#include "docksidebar_p.h"

#include <QtWidgets/QStyle>

#include "dockwidget_p.h"

namespace JBDS {

    static const char PROPERTY_HIGHLIGHT[] = "highlight";

    DockSideBar::DockSideBar(JBDS::DockWidget *dock, Qt::Edge edge, QWidget *parent)
        : QFrame(parent), m_dock(dock), m_edge(edge), m_widthHint(0) {
        switch (edge) {
            case Qt::TopEdge:
            case Qt::BottomEdge: {
                m_layout = new QHBoxLayout();
                m_firstLayout = new QBoxLayout(QBoxLayout::LeftToRight);
                m_secondLayout = new QBoxLayout(QBoxLayout::RightToLeft);
                setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

                m_buttonOrientation = Horizontal;
                break;
            }
            case Qt::LeftEdge:
            case Qt::RightEdge: {
                m_layout = new QVBoxLayout();
                m_firstLayout = new QBoxLayout(QBoxLayout::TopToBottom);
                m_secondLayout = new QBoxLayout(QBoxLayout::BottomToTop);
                setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

                m_buttonOrientation = (edge == Qt::LeftEdge) ? BottomToTop : TopToBottom;
                break;
            }
        }

        m_layout->setContentsMargins({});
        m_layout->setSpacing(0);

        m_layout->addLayout(m_firstLayout);
        m_layout->addStretch();
        m_layout->addLayout(m_secondLayout);

        setLayout(m_layout);
    }

    DockSideBar::~DockSideBar() {
    }

    QSize DockSideBar::sizeHint() const {
        QSize sz = QFrame::sizeHint();
        if (highlight()) {
            if (m_buttonOrientation == Horizontal) {
                if (sz.height() < m_widthHint)
                    sz.setHeight(m_widthHint);
            } else {
                if (sz.width() < m_widthHint)
                    sz.setWidth(m_widthHint);
            }
        }
        return sz;
    }

    void DockSideBar::insertButton(Side side, int index, QAbstractButton *button) {
        auto &cards = (side == Front) ? m_firstCards : m_secondCards;
        auto &layout = (side == Front) ? m_firstLayout : m_secondLayout;

        // Uncheck all other cards
        auto dock_p = DockWidgetPrivate::get(m_dock);
        if (dock_p->dockVisible(button)) {
            for (auto cur : cards) {
                if (dock_p->dockVisible(cur) && cur != button) {
                    cur->setChecked(false);
                }
            }
        }

        if (index >= cards.size() || index < 0) {
            layout->addWidget(button);
            cards.append(button);
        } else {
            layout->insertWidget(index, button);
            cards.insert(index, button);
        }

        button->show();

        // Transfer to dock
        dock_p->delegate->setButtonOrientation(button, m_buttonOrientation);
        dock_p->barButtonAdded(m_edge, side, button);
    }

    void DockSideBar::removeButton(Side side, QAbstractButton *button) {
        auto &cards = (side == Front) ? m_firstCards : m_secondCards;
        auto &layout = (side == Front) ? m_firstLayout : m_secondLayout;

        int cardIndex = cards.indexOf(button);
        if (cardIndex < 0) {
            return;
        }

        cards.removeAt(cardIndex);
        layout->removeWidget(button);

        // Transfer to dock
        auto dock_p = DockWidgetPrivate::get(m_dock);
        dock_p->barButtonRemoved(m_edge, side, button);
    }

    void DockSideBar::buttonToggled(Side side, QAbstractButton *button) {
        auto &cards = (side == Front) ? m_firstCards : m_secondCards;

        auto dock_p = DockWidgetPrivate::get(m_dock);
        if (dock_p->dockVisible(button)) {
            for (auto cur : cards) {
                if (dock_p->dockVisible(cur) && cur != button) {
                    cur->setChecked(false);
                }
            }
        }
    }

    void DockSideBar::buttonViewModeChanged(Side side, QAbstractButton *button) {
        auto &cards = (side == Front) ? m_firstCards : m_secondCards;

        auto dock_p = DockWidgetPrivate::get(m_dock);
        if (dock_p->dockVisible(button)) {
            for (auto cur : cards) {
                if (dock_p->dockVisible(cur) && cur != button) {
                    cur->setChecked(false);
                }
            }
        }
    }

    bool DockSideBar::highlight() const {
        return property(PROPERTY_HIGHLIGHT).toBool();
    }

    void DockSideBar::setHighlight(bool highlight, int widthHint) {
        if (this->highlight() != highlight) {
            setProperty(PROPERTY_HIGHLIGHT, highlight);
            style()->polish(this);
        } else if (highlight) {
            return;
        }
        if (highlight) {
            m_widthHint = widthHint;
        } else {
            m_widthHint = 0;
        }
        updateGeometry();
    }

    int DockSideBar::buttonSpacing() const {
        return m_firstLayout->spacing();
    }

    void DockSideBar::setButtonSpacing(int spacing) {
        m_firstLayout->setSpacing(spacing);
        m_secondLayout->setSpacing(spacing);
    }

}