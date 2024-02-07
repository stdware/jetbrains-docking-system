#include "dockdragcontroller_p.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtGui/QtEvents>
#include <QtGui/QScreen>

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

    class DockDragLabel : public QLabel {
    public:
        explicit DockDragLabel(QWidget *parent = nullptr) : QLabel(parent) {
        }
        ~DockDragLabel() = default;

        QAbstractButton *currentCard;
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

        auto label = new DockDragLabel(m_dock);
        m_label = label;
        label->setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);

        label->currentCard = button;
        label->currentPos = pos;
        label->originBar = orgSidebar;
        label->targetBar = nullptr;

        label->setPixmap(pixmap);
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

        auto currentCard = label->currentCard;
        auto d = DockWidgetPrivate::get(m_dock);

        DockSideBar *targetBar = nullptr;
        for (auto bar : d->bars) {
            if (!targetBar && bar->isEnabled() && bar->isVisible()) {
                int widthHint = d->delegate->buttonOrientation(currentCard) == Horizontal
                                    ? currentCard->sizeHint().height()
                                    : currentCard->sizeHint().width();
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

    static int cardAtWidget(DockSideBar *sideBar, Side side, QWidget *w, bool reverse = false) {
        auto cards = sideBar->buttons(side);
        int index = 0;
        QPoint center = w->mapToGlobal(w->rect().center());
        for (auto card : cards) {
            QPoint pos = card->mapToGlobal(QPoint(0, 0));
            if (sideBar->orientation() == Qt::Vertical) {
                if (reverse) {
                    if (pos.y() < center.y()) {
                        break;
                    }
                } else {
                    if (pos.y() + card->height() > center.y()) {
                        break;
                    }
                }
            } else {
                if (reverse) {
                    if (pos.x() < center.x()) {
                        break;
                    }
                } else {
                    if (pos.x() + card->width() > center.x()) {
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
        auto card = label->currentCard;

        auto d = DockWidgetPrivate::get(m_dock);
        auto &data = d->buttonDataHash.value(card);

        if (auto sideBar = label->targetBar) {
            if (sideBar->orientation() == Qt::Horizontal) {
                int barX = sideBar->mapToGlobal({}).x();
                int leftPos = barX + sideBar->layoutGeometry(Front).width();
                int rightPos = barX + sideBar->width() - sideBar->layoutGeometry(Back).width();

                if (label->x() - leftPos < rightPos - (label->x() + label->width())) {
                    int index = cardAtWidget(sideBar, Front, label);
                    m_dock->moveWidget(card, sideBar->edge(), Front, index);
                } else {
                    int index = cardAtWidget(sideBar, Back, label, true);
                    m_dock->moveWidget(card, sideBar->edge(), Back, index);
                }
            } else {
                int barY = sideBar->mapToGlobal({}).y();
                int topPos = barY + sideBar->layoutGeometry(Front).height();
                int bottomPos = barY + sideBar->height() - sideBar->layoutGeometry(Back).height();

                sideBar->removeButton(data.side, card);
                if (label->y() - topPos < bottomPos - (label->y() + label->height())) {
                    int index = cardAtWidget(sideBar, Front, label);
                    m_dock->moveWidget(card, sideBar->edge(), Front, index);
                } else {
                    int index = cardAtWidget(sideBar, Back, label, true);
                    m_dock->moveWidget(card, sideBar->edge(), Back, index);
                }
            }
        } else {
            //            auto pos = QCursor::pos();
            //            QTimer::singleShot(0, card, [pos, card]() {
            //                if (card->viewMode() == DockPinned) {
            //                    card->setViewMode(CDockCard::Float);
            //                    card->moveWidget(pos);
            //                } else if (!card->isChecked()) {
            //                    card->moveWidget(pos);
            //                }
            //                card->setChecked(true);
            //            });
        }

        if (!card->isEnabled()) {
            card->removeEventFilter(this);
            card->setDisabled(false);
            card->update();
        }
        if (card->isHidden()) {
            card->show();
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