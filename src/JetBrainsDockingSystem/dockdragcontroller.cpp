// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#include "dockdragcontroller_p.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtGui/QtEvents>
#include <QtGui/QScreen>
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

#include "dockwidget_p.h"

namespace JBDS {

    static inline bool rectHitTest(const QRectF &rect1, const QRectF &rect2) {
        if (rect1.x() + rect1.width() > rect2.x() && rect2.x() + rect2.width() > rect1.x() &&
            rect1.y() + rect1.height() > rect2.y() && rect2.y() + rect2.height() > rect1.y()) {
            return true;
        }
        return false;
    }

    static inline bool rectHitTest(const QRect &rect1, const QRect &rect2) {
        return rectHitTest(QRectF(rect1), QRectF(rect2));
    }

    static bool widgetHitTest(QWidget *w1, QWidget *w2) {
        QRect rect1 = QRect(w1->mapToGlobal(QPoint(0, 0)), w1->size());
        QRect rect2 = QRect(w2->mapToGlobal(QPoint(0, 0)), w2->size());
        return rectHitTest(rect1, rect2);
    }

    class DockDragLabel : public QWidget {
    public:
        explicit DockDragLabel(const QPixmap &pixmap, QWidget *parent = nullptr)
            : QWidget(parent), pixmap(pixmap) {
        }
        ~DockDragLabel() = default;

        QSize sizeHint() const override {
            return pixmap.size() / pixmap.devicePixelRatio();
        }

        void paintEvent(QPaintEvent *) override {
            QPainter painter(this);
            QStyleOption opt;
            opt.initFrom(this);
            style()->drawItemPixmap(&painter, rect(), Qt::AlignCenter, pixmap);
        }

        QPixmap pixmap;
        QAbstractButton *currentButton;
        QPoint currentPos;
        DockSideBar *originBar, *targetBar;
    };

    DockDragController::DockDragController(DockWidget *dock, QObject *parent)
        : QObject(parent), m_dock(dock), m_label() {
    }

    DockDragController::~DockDragController() {
        delete m_label;
    }

    void DockDragController::startDrag(QAbstractButton *button, const QPoint &pos,
                                       const QPixmap &pixmap) {
        auto dock_p = DockWidgetPrivate::get(m_dock);
        auto data = dock_p->buttonDataHash.value(button);
        auto orgSidebar = dock_p->bars[edge2index(data.edge)];
        if (orgSidebar->count(Front) + orgSidebar->count(Back) == 1) {
            button->setDisabled(true);
            button->installEventFilter(this);
        } else {
            button->hide();
        }

        auto label = new DockDragLabel(pixmap, m_dock);
        m_label = label;
        label->setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);

        label->currentButton = button;
        label->currentPos = pos;
        label->originBar = orgSidebar;
        label->targetBar = nullptr;

        label->adjustSize();
        label->setFocus();
        label->installEventFilter(this);
        label->grabMouse();

        tabDragMove();
        label->show();
    }

    bool DockDragController::eventFilter(QObject *obj, QEvent *event) {
        if (obj == m_label) {
            switch (event->type()) {
                case QEvent::MouseMove:
                    tabDragMove();
                    return true;
                case QEvent::FocusOut:
                case QEvent::MouseButtonRelease:
                    tabDragOver();
                    return true;
                default:
                    break;
            }
        } else {
            if (event->type() == QEvent::Paint) {
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }

    void DockDragController::tabDragMove() {
        auto label = static_cast<DockDragLabel *>(m_label);
        label->move(QCursor::pos() - label->currentPos);

        auto button = label->currentButton;
        auto d = DockWidgetPrivate::get(m_dock);

        DockSideBar *targetBar = nullptr;
        for (auto bar : d->bars) {
            if (!targetBar && bar->isEnabled() && bar->isVisible()) {
                int widthHint = d->delegate->buttonOrientation(button) == Horizontal
                                    ? button->sizeHint().height()
                                    : button->sizeHint().width();
                bool hit;
                if (bar->count(Front) + bar->count(Back) == 0) {
                    QPoint pos;
                    QSize size;
                    if (bar == d->bars[0]) {
                        pos = bar->mapToGlobal(QPoint(0, 0));
                        size = QSize(widthHint, bar->height());
                    } else if (bar == d->bars[2]) {
                        pos = bar->mapToGlobal(QPoint(bar->width() - widthHint, 0));
                        size = QSize(widthHint, bar->height());
                    } else if (bar == d->bars[1]) {
                        pos = bar->mapToGlobal(QPoint(0, 0));
                        size = QSize(bar->width(), widthHint);
                    } else {
                        pos = bar->mapToGlobal(QPoint(0, bar->height() - widthHint));
                        size = QSize(bar->width(), widthHint);
                    }
                    QRect rect(pos, size);
                    hit = rectHitTest(rect, QRect(label->mapToGlobal(QPoint(0, 0)), label->size()));
                } else {
                    hit = widgetHitTest(bar, label);
                }
                if (hit) {
                    bar->setHighlight(bar != label->originBar, widthHint);
                    targetBar = bar;
                } else {
                    bar->setHighlight(false);
                }
            } else {
                bar->setHighlight(false);
            }
        }
        label->targetBar = targetBar;
    }

    static int buttonAtWidget(DockSideBar *sideBar, Side side, QWidget *w, bool reverse = false) {
        auto buttons = sideBar->buttons(side);
        auto halfSpacing = sideBar->buttonLayout(side)->spacing() / 2;

        int index = 0;
        QPoint center = w->mapToGlobal(w->rect().center());
        for (auto button : buttons) {
            QPoint pos = button->mapToGlobal(QPoint(0, 0));
            if (sideBar->orientation() == Qt::Vertical) {
                if (reverse) {
                    if (pos.y() - halfSpacing < center.y()) {
                        break;
                    }
                } else {
                    if (pos.y() + button->height() + halfSpacing > center.y()) {
                        break;
                    }
                }
            } else {
                if (reverse) {
                    if (pos.x() - halfSpacing < center.x()) {
                        break;
                    }
                } else {
                    if (pos.x() + button->width() + halfSpacing > center.x()) {
                        break;
                    }
                }
            }
            index++;
        }
        return index;
    };

    void DockDragController::tabDragOver() {
        auto label = static_cast<DockDragLabel *>(m_label);
        auto button = label->currentButton;

        auto d = DockWidgetPrivate::get(m_dock);
        auto data = d->buttonDataHash.value(button);

        if (auto sideBar = label->targetBar) {
            if (sideBar->orientation() == Qt::Horizontal) {
                int barX = sideBar->mapToGlobal(QPoint()).x();
                int leftPos = barX + sideBar->buttonLayout(Front)->geometry().width();
                int rightPos =
                    barX + sideBar->width() - sideBar->buttonLayout(Back)->geometry().width();

                if (label->x() - leftPos < rightPos - (label->x() + label->width())) {
                    int index = buttonAtWidget(sideBar, Front, label);
                    m_dock->moveWidget(button, sideBar->edge(), Front, index);
                } else {
                    int index = buttonAtWidget(sideBar, Back, label, true);
                    m_dock->moveWidget(button, sideBar->edge(), Back, index);
                }
            } else {
                int barY = sideBar->mapToGlobal(QPoint()).y();
                int topPos = barY + sideBar->buttonLayout(Front)->geometry().height();
                int bottomPos =
                    barY + sideBar->height() - sideBar->buttonLayout(Back)->geometry().height();

                sideBar->removeButton(data.side, button);
                if (label->y() - topPos < bottomPos - (label->y() + label->height())) {
                    int index = buttonAtWidget(sideBar, Front, label);
                    m_dock->moveWidget(button, sideBar->edge(), Front, index);
                } else {
                    int index = buttonAtWidget(sideBar, Back, label, true);
                    m_dock->moveWidget(button, sideBar->edge(), Back, index);
                }
            }
        } else if (d->attributes[DockWidget::AutoFloatDraggingOutside]) {
            auto pos = QCursor::pos();
            auto widget = data.widget;
            auto viewMode = data.viewMode;
            QTimer::singleShot(0, button, [pos, button, widget, viewMode, this]() {
                if (viewMode == DockPinned) {
                    m_dock->setViewMode(button, Floating);
                    DockWidgetPrivate::moveWidgetToPos(widget, pos);
                } else if (!button->isChecked()) {
                    DockWidgetPrivate::moveWidgetToPos(widget, pos);
                }
                button->setChecked(true);
            });
        }

        if (!button->isEnabled()) {
            button->removeEventFilter(this);
            button->setDisabled(false);
            button->update();
        }
        if (button->isHidden()) {
            button->show();
        }

        label->removeEventFilter(this);
        label->releaseMouse();
        label->deleteLater(); // Don't delete directly
        m_label = nullptr;

        for (auto bar : std::as_const(d->bars)) {
            bar->setHighlight(false);
        }
    }

}