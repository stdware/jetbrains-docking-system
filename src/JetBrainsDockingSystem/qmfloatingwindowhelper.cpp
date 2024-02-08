#include "qmfloatingwindowhelper_p.h"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>

class QMFloatingWindowHelperPrivate : public QObject {
public:
    QMFloatingWindowHelperPrivate(QWidget *w, QMFloatingWindowHelper *q);
    ~QMFloatingWindowHelperPrivate();

    QMFloatingWindowHelper *q;

    void setFloating_helper(bool floating, Qt::WindowFlags flags);

    enum EdgeAndCorner {
        None = -1,
        Left = 0,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        SizeOfEdgeAndCorner,
    };

    QWidget *w;
    QMargins m_resizeMargins;

    bool m_floating;
    int m_windowFlags;


    QPoint m_pressedPos;
    QRect m_orgGeometry;
    QRect m_rect;

    Qt::MouseButton m_pressedButton;
    EdgeAndCorner m_pressedArea;
    QRect m_pressedRect[SizeOfEdgeAndCorner];

    bool dummyEventFilter(QObject *obj, QEvent *event);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    friend class QMFloatingWindowHelper;
};

static bool isDescendantOfWidget(QWidget *widget, QWidget *parent) {
    auto upper = parent->parentWidget();
    while (widget && widget != upper) {
        if (widget == parent)
            return true;
        widget = widget->parentWidget();
    }
    return false;
}

QMFloatingWindowHelperPrivate::QMFloatingWindowHelperPrivate(QWidget *w, QMFloatingWindowHelper *q)
    : q(q), w(w) {
    m_resizeMargins = {5, 5, 5, 5};

    m_floating = false;
    m_windowFlags = 0;
    m_pressedButton = Qt::NoButton;
    m_pressedArea = None;

    // Initialize array
    for (auto &rec : m_pressedRect) {
        rec = {0, 0, 0, 0};
    }

    w->setAttribute(Qt::WA_Hover);
}

QMFloatingWindowHelperPrivate::~QMFloatingWindowHelperPrivate() {
}

void QMFloatingWindowHelperPrivate::setFloating_helper(bool floating, Qt::WindowFlags flags) {
    m_floating = floating;

    if (floating) {
        m_windowFlags = w->windowFlags();
        w->setWindowFlags(flags | Qt::FramelessWindowHint);
        qApp->installEventFilter(this);
    } else {
        qApp->removeEventFilter(this);
        w->setWindowFlags(static_cast<Qt::WindowFlags>(m_windowFlags));
        m_windowFlags = 0;
    }
}

bool QMFloatingWindowHelperPrivate::dummyEventFilter(QObject *obj, QEvent *event) {
    switch (event->type()) {
        case QEvent::Show:
        case QEvent::Resize: {
            // Re-calc anchor areas
            int width = w->width();
            int height = w->height();

            int left = m_resizeMargins.left();
            int right = m_resizeMargins.right();
            int top = m_resizeMargins.top();
            int bottom = m_resizeMargins.bottom();

            m_pressedRect[Left] = QRect(0, top, left, height - top - bottom);
            m_pressedRect[Right] = QRect(width - right, top, right, height - top - bottom);
            m_pressedRect[Top] = QRect(left, 0, width - left - right, top);
            m_pressedRect[Bottom] = QRect(left, height - bottom, width - left - right, bottom);

            m_pressedRect[TopLeft] = QRect(0, 0, left, top);
            m_pressedRect[TopRight] = QRect(width - right, 0, right, top);
            m_pressedRect[BottomLeft] = QRect(0, height - bottom, left, bottom);
            m_pressedRect[BottomRight] = QRect(width - right, height - bottom, right, bottom);

            break;
        }
        case QEvent::HoverMove: {
            auto e = static_cast<QHoverEvent *>(event);
            QPoint pos = e->pos();

            // Set cursor
            if (m_pressedRect[Left].contains(pos) || m_pressedRect[Right].contains(pos)) {
                w->setCursor(Qt::SizeHorCursor);
            } else if (m_pressedRect[Top].contains(pos) || m_pressedRect[Bottom].contains(pos)) {
                w->setCursor(Qt::SizeVerCursor);
            } else if (m_pressedRect[TopLeft].contains(pos) ||
                       m_pressedRect[BottomRight].contains(pos)) {
                w->setCursor(Qt::SizeFDiagCursor);
            } else if (m_pressedRect[TopRight].contains(pos) ||
                       m_pressedRect[BottomLeft].contains(pos)) {
                w->setCursor(Qt::SizeBDiagCursor);
            } else {
                w->setCursor(Qt::ArrowCursor);
            }

            if (m_pressedButton != Qt::LeftButton)
                break;

            // Calc the movement by mouse pos
            auto globalPos = QCursor::pos();
            int offsetX = globalPos.x() - m_pressedPos.x();
            int offsetY = globalPos.y() - m_pressedPos.y();

            const int &rectX = m_orgGeometry.x();
            const int &rectY = m_orgGeometry.y();
            const int &rectW = m_orgGeometry.width();
            const int &rectH = m_orgGeometry.height();

            auto minSize = w->minimumSizeHint();
            minSize.rwidth() = qMax(minSize.width(), w->minimumWidth());
            minSize.rheight() = qMax(minSize.height(), w->minimumHeight());

            auto maxSize = w->maximumSize();

            auto curRect = m_rect;

            // Execute stretch or move
            switch (m_pressedArea) {
                case Left: {
                    int resizeW = rectW - offsetX;
                    if (resizeW >= minSize.width() && resizeW <= maxSize.width()) {
                        curRect.setRect(rectX + offsetX, curRect.y(), resizeW, curRect.height());
                    }
                    break;
                }
                case Right: {
                    int resizeW = rectW + offsetX;
                    if (resizeW >= minSize.width() && resizeW <= maxSize.width()) {
                        curRect.setRect(rectX, curRect.y(), resizeW, curRect.height());
                    }
                    break;
                }
                case Top: {
                    int resizeH = rectH - offsetY;
                    if (resizeH >= minSize.height() && resizeH <= maxSize.height()) {
                        curRect.setRect(curRect.x(), rectY + offsetY, curRect.width(), resizeH);
                    }
                    break;
                }
                case Bottom: {
                    int resizeH = rectH + offsetY;
                    if (resizeH >= minSize.height() && resizeH <= maxSize.height()) {
                        curRect.setRect(curRect.x(), rectY, curRect.width(), resizeH);
                    }
                    break;
                }
                case TopLeft: {
                    int resizeW = rectW - offsetX;
                    int resizeH = rectH - offsetY;
                    if (resizeW >= minSize.width() && resizeW <= maxSize.width()) {
                        curRect.setRect(rectX + offsetX, curRect.y(), resizeW, curRect.height());
                    }
                    if (resizeH >= minSize.height() && resizeH <= maxSize.height()) {
                        curRect.setRect(curRect.x(), rectY + offsetY, curRect.width(), resizeH);
                    }
                    break;
                }
                case TopRight: {
                    int resizeW = rectW + offsetX;
                    int resizeH = rectH - offsetY;
                    if (resizeW >= minSize.width() && resizeW <= maxSize.width()) {
                        curRect.setRect(rectX, curRect.y(), resizeW, curRect.height());
                    }
                    if (resizeH >= minSize.height() && resizeH <= maxSize.height()) {
                        curRect.setRect(curRect.x(), rectY + offsetY, curRect.width(), resizeH);
                    }
                    break;
                }
                case BottomLeft: {
                    int resizeW = rectW - offsetX;
                    int resizeH = rectH + offsetY;
                    if (resizeW >= minSize.width() && resizeW <= maxSize.width()) {
                        curRect.setRect(rectX + offsetX, curRect.y(), resizeW, curRect.height());
                    }
                    if (resizeH >= minSize.height() && resizeH <= maxSize.height()) {
                        curRect.setRect(curRect.x(), rectY, curRect.width(), resizeH);
                    }
                    break;
                }
                case BottomRight: {
                    int resizeW = rectW + offsetX;
                    int resizeH = rectH + offsetY;
                    if (resizeW >= minSize.width() && resizeW <= maxSize.width()) {
                        curRect.setRect(rectX, curRect.y(), resizeW, curRect.height());
                    }
                    if (resizeH >= minSize.height() && resizeH <= maxSize.height()) {
                        curRect.setRect(curRect.x(), rectY, curRect.width(), resizeH);
                    }
                    break;
                }
                default: {
                    curRect.moveTopLeft(QPoint(rectX + offsetX, rectY + offsetY));
                    break;
                }
            }

            if (curRect != m_rect) {
                m_rect = curRect;
                w->setGeometry(curRect);
            }
            break;
        }
        case QEvent::MouseButtonPress: {
            auto e = static_cast<QMouseEvent *>(event);
            m_pressedButton = Qt::NoButton;

            // Record mouse press coordinates
            auto pos = e->pos();

            bool pressed = false;
            for (int i = 0; i < SizeOfEdgeAndCorner; ++i) {
                if (m_pressedRect[i].contains(pos)) {
                    m_pressedArea = static_cast<EdgeAndCorner>(i);
                    pressed = true;
                    break;
                }
            }

            if (!pressed) {
                auto widget = qobject_cast<QWidget *>(obj);
                if (widget && (widget == w || widget->property("jbds-titlebar").toBool())) {
                    pressed = true;
                }
            }

            if (pressed) {
                m_pressedButton = e->button();
                m_pressedPos = QCursor::pos();
                m_orgGeometry = w->geometry();
                m_rect = m_orgGeometry;
                return true;
            }
            break;
        }
        case QEvent::Leave: {
            w->setCursor(Qt::ArrowCursor);
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto e = static_cast<QMouseEvent *>(event);

            bool pressed = m_pressedButton != Qt::NoButton;

            // Restore all
            w->setCursor(Qt::ArrowCursor);
            m_pressedButton = Qt::NoButton;
            m_pressedArea = None;

            if (pressed) {
                return true;
            }

            break;
        }
        default:
            break;
    }
    return false;
}

bool QMFloatingWindowHelperPrivate::eventFilter(QObject *obj, QEvent *event) {
    if (obj == w) {
        if (dummyEventFilter(obj, event)) {
            return true;
        }
    } else {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease: {
                if (obj->isWidgetType()) {
                    auto e = static_cast<QMouseEvent *>(event);
                    auto widget = qobject_cast<QWidget *>(obj);
                    if (widget && isDescendantOfWidget(widget, w)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                        auto me = QMouseEvent(e->type(), w->mapFromGlobal(e->globalPosition()),
                                              w->window()->mapFromGlobal(e->globalPosition()),
                                              e->globalPosition(), e->button(), e->buttons(),
                                              e->modifiers(), e->source(),
                                              dynamic_cast<const QPointingDevice *>(e->device()));
                        me.setTimestamp(e->timestamp());
#else
                        auto me = *e;
                        me.setLocalPos(w->mapFromGlobal(e->globalPos()));
#endif
                        if (dummyEventFilter(obj, &me)) {
                            return true;
                        }
                    }
                }
            }
            default:
                break;
        }
    }
    return QObject::eventFilter(obj, event);
}

QMFloatingWindowHelper::QMFloatingWindowHelper(QWidget *w, QObject *parent)
    : QObject(parent), d(new QMFloatingWindowHelperPrivate(w, this)) {
}

QMFloatingWindowHelper::~QMFloatingWindowHelper() {
    delete d;
}

bool QMFloatingWindowHelper::floating() const {
    return d->m_floating;
}

void QMFloatingWindowHelper::setFloating(bool floating, Qt::WindowFlags flags) {
    if (d->m_floating == floating)
        return;
    d->setFloating_helper(floating, flags);
}

QMargins QMFloatingWindowHelper::resizeMargins() const {
    return d->m_resizeMargins;
}

void QMFloatingWindowHelper::setResizeMargins(const QMargins &resizeMargins) {
    d->m_resizeMargins = resizeMargins;
}