// Copyright (C) 2023-2024 Stdware Collections (https://www.github.com/stdware)
// SPDX-License-Identifier: MIT

#include "dockwidget.h"
#include "dockwidget_p.h"

#include <QtCore/QTimer>
#include <QtGui/QtEvents>
#include <QtGui/QWindow>
#include <QtGui/QPixmap>
#include <QtGui/QGuiApplication>

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)

#  include <private/qapplication_p.h>

static inline QWidget *appActiveWindow() {
    return QApplicationPrivate::active_window;
}

static inline void setAppActiveWindow(QWidget *w) {
    QApplicationPrivate::active_window = w;
}

#else

#  include <private/qguiapplication_p.h>

class QApplicationPrivate : public QGuiApplicationPrivate {
public:
    static inline QWidget *activeWindow2() {
        return active_window;
    }

    static inline void setActiveWindow2(QWidget *w) {
        active_window = w;
    }

private:
    Q_DECL_IMPORT static QApplicationPrivate *self;
    Q_DECL_IMPORT static QWidget *active_window;
};

static inline QWidget *appActiveWindow() {
    return QApplicationPrivate::activeWindow2();
}

static inline void setAppActiveWindow(QWidget *w) {
    QApplicationPrivate::setActiveWindow2(w);
}

#endif

#include "dockbutton.h"

#include "qmfloatingwindowhelper_p.h"

namespace JBDS {

    static QPixmap createPixmap(const QSize &logicalPixelSize, QWindow *window) {
#ifndef Q_OS_MACOS
        qreal targetDPR = window ? window->devicePixelRatio() : qApp->devicePixelRatio();
        QPixmap ret(logicalPixelSize * targetDPR);
        ret.setDevicePixelRatio(targetDPR);
        return ret;
#else
        Q_UNUSED(window);
        return QPixmap(logicalPixelSize);
#endif
    }

    static QPixmap buttonShot(QAbstractButton *button) {
        QPixmap pixmap = QGuiApplication::testAttribute(Qt::AA_UseHighDpiPixmaps)
                             ? createPixmap(button->size(), button->window()->windowHandle())
                             : QPixmap(button->size());
        pixmap.fill(Qt::transparent);
        button->render(&pixmap);
        return pixmap;
    }

    static void adjustWindowGeometry(QWidget *w) {
        auto screen = w->screen();
        auto screenGeometry = screen->geometry();
        if (w->x() < screenGeometry.left() || w->y() < screenGeometry.top()) {
            w->move(qMax(w->x(), screenGeometry.left()), qMax(w->y(), screenGeometry.top()));
        }
        if (w->x() + w->width() > screenGeometry.right() ||
            w->y() + w->height() > screenGeometry.bottom()) {
            w->move(qMin(w->x(), screenGeometry.right() - w->width()),
                    qMin(w->y(), screenGeometry.bottom() - w->height()));
        }
    }

    class ShortcutFilter : public QObject {
    public:
        ShortcutFilter(QWidget *org) : m_org(org), m_handled(false) {
        }

        inline bool handled() const {
            return m_handled;
        }

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override {
            if (event->type() == QEvent::Shortcut) {
                setAppActiveWindow(m_org);
                m_handled = true;
            }
            return QObject::eventFilter(watched, event);
        }

    private:
        QWidget *m_org;
        bool m_handled;
    };

    class WidgetEventFilter : public QObject {
    public:
        WidgetEventFilter(DockWidgetPrivate *d, QWidget *w, QAbstractButton *button,
                          QObject *parent = nullptr)
            : QObject(parent), d(d), widget(w), button(button), closing(false) {
            widget->installEventFilter(this);
        }

        DockWidgetPrivate *d;
        QWidget *widget;
        QAbstractButton *button;
        QRect oldGeometry;
        bool closing;

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::Close: {
                    closing = true;
                    QTimer::singleShot(0, this, [this]() {
                        closing = false; //
                    });
                    break;
                }
                case QEvent::Hide: {
                    if (closing) {
                        // Close accepted
                        button->setChecked(false);
                        if (d->buttonDataHash.value(button).viewMode == DockPinned) {
                            QTimer::singleShot(0, this, [this]() {
                                widget->show(); //
                            });
                        }
                    }
                    break;
                }
                case QEvent::Show:
                case QEvent::Move:
                case QEvent::Resize: {
                    if (d->buttonDataHash.value(button).viewMode != DockPinned) {
                        oldGeometry = widget->geometry();
                    }
                    break;
                }
                case QEvent::KeyPress: {
                    if (!widget->isWindow()) {
                        break;
                    }

                    auto e = static_cast<QKeyEvent *>(event);
                    e->accept();

                    // Hack `active_window` temporarily
                    auto org = appActiveWindow();
                    setAppActiveWindow(button->window());

                    // Make sure to restore `active_window` right away if shortcut matches
                    ShortcutFilter filter(org);
                    qApp->installEventFilter(&filter);

                    // Retransmit event
                    QKeyEvent keyEvent(QEvent::ShortcutOverride, e->key(), e->modifiers(),
                                       e->nativeScanCode(), e->nativeVirtualKey(),
                                       e->nativeModifiers(), e->text(), e->isAutoRepeat(),
                                       e->count());
                    QGuiApplicationPrivate::instance()->shortcutMap.tryShortcut(&keyEvent);

                    if (!filter.handled()) {
                        setAppActiveWindow(org);
                    }
                }
                default:
                    break;
            }
            return QObject::eventFilter(obj, event);
        }
    };

    class ButtonEventFilter : public QObject {
    public:
        ButtonEventFilter(DockWidgetPrivate *d, QWidget *w, QAbstractButton *button,
                          QObject *parent = nullptr)
            : QObject(parent), d(d), widget(w), button(button), readyDrag(false) {
            button->installEventFilter(this);
        }

        DockWidgetPrivate *d;
        QWidget *widget;
        QAbstractButton *button;

        QPoint dragPos;
        QSize dragOffset = QSize(10, 10);
        bool readyDrag;

    protected:
        void mousePressEvent(QMouseEvent *event) {
            if (event->button() == Qt::LeftButton) {
                dragPos = event->pos();
                readyDrag = true;
            }
        }

        void mouseMoveEvent(QMouseEvent *event) {
            if (readyDrag) {
                QPoint pos = event->pos();
                if (qAbs(pos.x() - dragPos.x()) >= dragOffset.width() ||
                    qAbs(pos.y() - dragPos.y()) >= dragOffset.height()) {
                    readyDrag = false;

                    // Make sure the button receives the event first, otherwise the
                    // hover state will remain
                    QTimer::singleShot(0, this, [this]() {
                        d->dragCtl->startDrag(button, dragPos, buttonShot(button));
                    });
                }
            }
        }

        void mouseReleaseEvent(QMouseEvent *event) {
            if (readyDrag) {
                readyDrag = false;
            }
        }

        void leaveEvent(QEvent *event) {
            if (readyDrag) {
                readyDrag = false;
            }
        }

        void contextMenuEvent(QContextMenuEvent *event) {
            if (!d->attributes[DockWidget::ViewModeContextMenu]) {
                return;
            }

            auto menu = d->delegate->createViewModeMenu(button, button);
            auto data = d->buttonDataHash.value(button);

            QAction dockPinned(
                QCoreApplication::translate("JetBrainsDockingSystem", "Dock Pinned"));
            dockPinned.setCheckable(true);
            dockPinned.setChecked(data.viewMode == DockPinned);

            QAction floating(QCoreApplication::translate("JetBrainsDockingSystem", "Floating"));
            floating.setCheckable(true);
            floating.setChecked(data.viewMode == Floating);

            QAction window(QCoreApplication::translate("JetBrainsDockingSystem", "Window"));
            window.setCheckable(true);
            window.setChecked(data.viewMode == Window);

            menu->addAction(&dockPinned);
            menu->addAction(&floating);
            menu->addAction(&window);

            menu->deleteLater();

            auto action = menu->exec(QCursor::pos());
            ViewMode mode;
            if (action == &dockPinned) {
                mode = DockPinned;
            } else if (action == &floating) {
                mode = Floating;
            } else if (action == &window) {
                mode = Window;
            } else {
                return;
            }
            d->q_ptr->setViewMode(button, mode);
        }

        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::MouseButtonPress:
                    mousePressEvent(static_cast<QMouseEvent *>(event));
                    break;

                case QEvent::MouseMove:
                    mouseMoveEvent(static_cast<QMouseEvent *>(event));
                    break;

                case QEvent::MouseButtonRelease:
                    mouseReleaseEvent(static_cast<QMouseEvent *>(event));
                    break;

                case QEvent::Leave:
                    leaveEvent(event);
                    break;

                case QEvent::ContextMenu:
                    contextMenuEvent(static_cast<QContextMenuEvent *>(event));
                    return true;

                default:
                    break;
            }
            return QObject::eventFilter(obj, event);
        }
    };

    DockWidgetPrivate::DockWidgetPrivate() {
    }

    DockWidgetPrivate::~DockWidgetPrivate() {
        for (auto it = widgetIndexes.begin(); it != widgetIndexes.end(); ++it) {
            const auto &w = it.key();
            const auto &button = it.value();
            disconnect(w, &QObject::destroyed, this, &DockWidgetPrivate::_q_widgetDestroyed);
            disconnect(button, &QObject::destroyed, this, &DockWidgetPrivate::_q_buttonDestroyed);
        }
    }

    void DockWidgetPrivate::init() {
        Q_Q(DockWidget);

        q->setAttribute(Qt::WA_StyledBackground);

        mainLayout = new QGridLayout();
        mainLayout->setContentsMargins({});
        mainLayout->setSpacing(0);

        bars[0] = new DockSideBar(q, Qt::LeftEdge);
        bars[0]->setObjectName("left-bar");
        bars[1] = new DockSideBar(q, Qt::TopEdge);
        bars[1]->setObjectName("top-bar");
        bars[2] = new DockSideBar(q, Qt::RightEdge);
        bars[2]->setObjectName("right-bar");
        bars[3] = new DockSideBar(q, Qt::BottomEdge);
        bars[3]->setObjectName("bottom-bar");

        panels[0] = new DockPanel(Qt::Vertical);
        panels[1] = new DockPanel(Qt::Horizontal);
        panels[2] = new DockPanel(Qt::Vertical);
        panels[3] = new DockPanel(Qt::Horizontal);

        centralContainer = new QStackedWidget();

        /*
         *   0   1   2
         * 0   t t t
         *   l · · · r
         *
         * 1 l · * · r
         *
         *   l · · · r
         * 2   b b b
         *
         */

        horizontalSplitter = new QSplitter(Qt::Horizontal);
        horizontalSplitter->setObjectName("dock-splitter");
        horizontalSplitter->setChildrenCollapsible(false);
        horizontalSplitter->addWidget(panels[0]);
        horizontalSplitter->addWidget(centralContainer);
        horizontalSplitter->addWidget(panels[2]);

        horizontalSplitter->setStretchFactor(0, 0);
        horizontalSplitter->setStretchFactor(1, 1);
        horizontalSplitter->setStretchFactor(2, 0);

        verticalSplitter = new QSplitter(Qt::Vertical);
        verticalSplitter->setObjectName("dock-splitter");
        verticalSplitter->setChildrenCollapsible(false);
        verticalSplitter->addWidget(panels[1]);
        verticalSplitter->addWidget(horizontalSplitter);
        verticalSplitter->addWidget(panels[3]);

        verticalSplitter->setStretchFactor(0, 0);
        verticalSplitter->setStretchFactor(1, 1);
        verticalSplitter->setStretchFactor(2, 0);

        mainLayout->addWidget(bars[0], 1, 0);
        mainLayout->addWidget(bars[1], 0, 1);
        mainLayout->addWidget(bars[2], 1, 2);
        mainLayout->addWidget(bars[3], 2, 1);
        mainLayout->addWidget(verticalSplitter, 1, 1);

        q->setLayout(mainLayout);

        dragCtl.reset(new DockDragController(q));
    }

    void DockWidgetPrivate::barButtonAdded(Qt::Edge edge, Side side, QAbstractButton *button) {
        auto panel = panels[edge2index(edge)];
        auto data = buttonDataHash.value(button);
        panel->addWidget(side, data.container, dockVisible(button));
    }

    void DockWidgetPrivate::barButtonRemoved(Qt::Edge edge, Side side, QAbstractButton *button) {
        auto panel = panels[edge2index(edge)];
        auto data = buttonDataHash.value(button);
        panel->removeWidget(side, data.container);
    }

    void DockWidgetPrivate::moveWidgetToPos(QWidget *w, const QPoint &pos) {
        w->move(pos);
        adjustWindowGeometry(w);
    }

    void DockWidgetPrivate::_q_widgetDestroyed() {
        Q_Q(DockWidget);
        q->removeWidget(widgetIndexes.value(static_cast<QWidget *>(sender())));
    }

    void DockWidgetPrivate::_q_buttonDestroyed() {
        Q_Q(DockWidget);
        q->removeWidget(static_cast<QAbstractButton *>(sender()));
    }

    void DockWidgetPrivate::_q_buttonToggled(bool checked) {
        Q_UNUSED(checked)

        auto button = static_cast<QAbstractButton *>(sender());
        auto data = buttonDataHash.value(button);

        // Transfer to sidebar
        auto edgeIdx = edge2index(data.edge);
        bars[edgeIdx]->buttonToggled(data.side, button);

        // Transfer to panel
        auto panel = panels[edgeIdx];
        bool visible = button->isChecked();
        if (data.viewMode == DockPinned) {
            panel->setContainerVisible(data.side, visible);
            if (visible) {
                panel->setCurrentWidget(data.side, data.container);
            }
        } else {
            data.widget->setVisible(visible);
        }
    }

    DockWidget::DockWidget(QWidget *parent)
        : DockWidget(*new DockWidgetPrivate(), new DefaultDockButtonDelegate(), parent) {
    }

    DockWidget::DockWidget(DockButtonDelegate *delegate, QWidget *parent)
        : DockWidget(*new DockWidgetPrivate(), delegate, parent) {
    }

    DockWidget::~DockWidget() {
    }

    int DockWidget::resizeMargin() const {
        Q_D(const DockWidget);
        return d->resizeMargin;
    }

    void DockWidget::setResizeMargin(int resizeMargin) {
        Q_D(DockWidget);
        d->resizeMargin = resizeMargin;
        for (const auto &item : d->buttonDataHash) {
            static_cast<QMFloatingWindowHelper *>(item.floatingHelper)
                ->setResizeMargins({resizeMargin, resizeMargin, resizeMargin, resizeMargin});
        }
    }

    QWidget *DockWidget::widget() const {
        Q_D(const DockWidget);
        return (d->centralContainer->count() == 0) ? nullptr : d->centralContainer->widget(0);
    }

    void DockWidget::setWidget(QWidget *w) {
        Q_D(DockWidget);
        if (d->centralContainer->count() > 0) {
            delete takeWidget();
        }
        if (w) {
            d->centralContainer->addWidget(w);
        }
    }

    QWidget *DockWidget::takeWidget() {
        Q_D(DockWidget);
        if (d->centralContainer->count() > 0) {
            auto w = d->centralContainer->widget(0);
            d->centralContainer->removeWidget(w);
            return w;
        }
        return nullptr;
    }

    QAbstractButton *DockWidget::insertWidget(Qt::Edge edge, Side side, int index, QWidget *w) {
        Q_D(DockWidget);
        if (d->widgetIndexes.contains(w))
            return nullptr;

        // Create button
        auto button = d->delegate->create(nullptr);
        button->setCheckable(true);

        // Create container
        QWidget *container;
        {
            container = new QWidget();
            container->setObjectName("dock-widget-container");
            container->setAttribute(Qt::WA_StyledBackground);

            auto layout = new QVBoxLayout();
            layout->setContentsMargins({});
            layout->setSpacing(0);
            layout->addWidget(w);

            container->setLayout(layout);
        }

        auto floatingHelper = new QMFloatingWindowHelper(w, container);
        floatingHelper->setResizeMargins(
            {d->resizeMargin, d->resizeMargin, d->resizeMargin, d->resizeMargin});

        DockButtonData data{
            DockPinned,
            edge,
            side,
            w,
            container,
            floatingHelper,
            new WidgetEventFilter(d, w, button, container),
            new ButtonEventFilter(d, w, button, container),
        };

        // Add button data
        d->buttonDataHash.insert(button, data);
        d->widgetIndexes.insert(w, button);

        // Connect signals
        connect(w, &QObject::destroyed, d, &DockWidgetPrivate::_q_widgetDestroyed);
        connect(button, &QObject::destroyed, d, &DockWidgetPrivate::_q_buttonDestroyed);
        connect(button, &QAbstractButton::toggled, d, &DockWidgetPrivate::_q_buttonToggled);

        // Insert button
        d->bars[edge2index(edge)]->insertButton(data.side, index, button);

        return button;
    }

    void DockWidget::removeWidget(QAbstractButton *button) {
        Q_D(DockWidget);

        auto it = d->buttonDataHash.find(button);
        if (it == d->buttonDataHash.end())
            return;

        auto &data = it.value();
        auto w = data.widget;

        // Make the widget independent
        if (auto layout = data.container->layout(); layout->count() > 0) {
            layout->removeWidget(layout->itemAt(0)->widget());
        }

        // Remove button
        d->bars[edge2index(data.edge)]->removeButton(data.side, button);

        // Disconnect signals
        disconnect(w, &QObject::destroyed, d, &DockWidgetPrivate::_q_widgetDestroyed);
        disconnect(button, &QObject::destroyed, d, &DockWidgetPrivate::_q_buttonDestroyed);
        disconnect(button, &QAbstractButton::toggled, d, &DockWidgetPrivate::_q_buttonToggled);

        // Remove button and container
        button->deleteLater();
        data.container->deleteLater();

        // Remove button data
        d->widgetIndexes.remove(w);
        d->buttonDataHash.erase(it);
    }

    void DockWidget::moveWidget(QAbstractButton *button, Qt::Edge edge, Side side, int index) {
        Q_D(DockWidget);

        auto it = d->buttonDataHash.find(button);
        if (it == d->buttonDataHash.end())
            return;

        auto &data = it.value();
        auto orgBar = d->bars[edge2index(data.edge)];
        auto newBar = d->bars[edge2index(edge)];

        orgBar->removeButton(data.side, button);
        data.edge = edge;
        data.side = side;
        newBar->insertButton(side, index, button);
    }

    int DockWidget::widgetCount(Qt::Edge edge, Side side) const {
        Q_D(const DockWidget);
        return d->bars[edge2index(edge)]->count(side);
    }

    QList<QWidget *> DockWidget::widgets(Qt::Edge edge, Side side) const {
        Q_D(const DockWidget);

        auto buttons = d->bars[edge2index(edge)]->buttons(side);
        QList<QWidget *> res;
        res.reserve(buttons.size());
        for (const auto &button : std::as_const(buttons)) {
            res.append(d->buttonDataHash.value(button).widget);
        }
        return res;
    }

    QWidget *DockWidget::widget(const QAbstractButton *button) {
        Q_D(const DockWidget);
        return d->buttonDataHash.value(const_cast<QAbstractButton *>(button)).widget;
    }

    ViewMode DockWidget::viewMode(const QAbstractButton *button) {
        Q_D(const DockWidget);
        return d->buttonDataHash.value(const_cast<QAbstractButton *>(button)).viewMode;
    }

    void DockWidget::setViewMode(QAbstractButton *button, ViewMode viewMode) {
        Q_D(DockWidget);
        auto it = d->buttonDataHash.find(button);
        if (it == d->buttonDataHash.end())
            return;

        auto &data = it.value();
        auto oldViewMode = data.viewMode;
        if (oldViewMode == viewMode) {
            return;
        }

        auto widget = data.widget;
        auto container = data.container;
        auto edgeIdx = edge2index(data.edge);
        const auto &oldGeometry =
            static_cast<WidgetEventFilter *>(data.widgetEventFilter)->oldGeometry;
        auto floatingHelper = static_cast<QMFloatingWindowHelper *>(data.floatingHelper);
        auto asWindow = [&](const QSize &size, const QPoint &extraOffset) {
            // Size
            if (!size.isEmpty()) {
                widget->resize(size);
            } else if (!oldGeometry.isEmpty()) {
                widget->resize(oldGeometry.size());
            }

            // Pos
            if (!oldGeometry.isEmpty()) {
                widget->move(oldGeometry.topLeft() - extraOffset);
            } else {
                QPoint offset;
                switch (data.edge) {
                    case Qt::TopEdge:
                        offset.ry() += button->height();
                        break;
                    case Qt::BottomEdge:
                        offset.ry() -= button->height() + size.height();
                        break;
                    case Qt::LeftEdge:
                        offset.rx() += button->width();
                        break;
                    case Qt::RightEdge:
                        offset.rx() -= button->width() + size.width();
                        break;
                }

                DockWidgetPrivate::moveWidgetToPos(widget, button->mapToGlobal(QPoint()) + offset -
                                                               extraOffset);
            }

            widget->setVisible(button->isChecked());
        };

        auto layout = container->layout();
        switch (viewMode) {
            case DockPinned: {
                floatingHelper->setFloating(false);
                widget->setWindowFlags(Qt::Widget);
                layout->addWidget(widget);
                widget->setVisible(true);

                // Restore sizes
                if (!oldGeometry.isEmpty()) {
                    auto sideBar = d->bars[edgeIdx];
                    auto size = oldGeometry.size();
                    QTimer::singleShot(0, this, [viewMode, this, widget, sideBar, size]() {
                        switch (viewMode) {
                            case DockPinned: {
                                if (sideBar->orientation() == Qt::Horizontal) {
                                    setEdgeSize(sideBar->edge(), size.height());
                                } else {
                                    setEdgeSize(sideBar->edge(), size.width());
                                }
                                break;
                            }
                            default: {
                                widget->resize(size);
                                break;
                            }
                        }
                    });
                }
                break;
            }

            case Floating: {
                auto size = widget->size();
                auto extraOffset = oldViewMode == DockPinned
                                       ? QPoint()
                                       : (widget->mapToGlobal(QPoint()) - widget->pos());
                layout->removeWidget(widget);
                widget->setParent(container);
                floatingHelper->setFloating(true, Qt::Tool);
                asWindow(size, extraOffset);
                break;
            }

            case Window: {
                auto size = widget->size();
                auto extraOffset = oldViewMode == DockPinned
                                       ? QPoint()
                                       : (widget->mapToGlobal(QPoint()) - widget->pos());
                layout->removeWidget(widget);
                widget->setParent(nullptr);
                floatingHelper->setFloating(false);
                widget->setWindowFlags(Qt::Window);
                asWindow(size, extraOffset);
                break;
            }
        }

        data.viewMode = viewMode;

        // Transfer to bar
        d->bars[edgeIdx]->buttonViewModeChanged(data.side, button);

        // Update panels
        if (button->isChecked()) {
            auto panel = d->panels[edgeIdx];
            if (oldViewMode == DockPinned) {
                panel->setContainerVisible(data.side, false);
            } else if (viewMode == DockPinned) {
                panel->setContainerVisible(data.side, true);
                panel->setCurrentWidget(data.side, data.container);

                // We must refresh its layout
                layout->invalidate();
            }
        }
    }

    int DockWidget::edgeSize(Qt::Edge edge) const {
        Q_D(const DockWidget);
        switch (edge) {
            case Qt::TopEdge:
            case Qt::BottomEdge: {
                return d->panels[edge2index(edge)]->height();
            }
            case Qt::LeftEdge:
            case Qt::RightEdge: {
                return d->panels[edge2index(edge)]->width();
            }
        }
        return 0;
    }

    void DockWidget::setEdgeSize(Qt::Edge edge, int size) {
        Q_D(DockWidget);

        switch (edge) {
            case Qt::TopEdge: {
                auto sizes = d->verticalSplitter->sizes();
                auto offset = size - sizes[0];
                sizes[0] += offset;
                sizes[1] -= offset;
                d->verticalSplitter->setSizes(sizes);
                break;
            }
            case Qt::BottomEdge: {
                auto sizes = d->verticalSplitter->sizes();
                auto offset = size - sizes[2];
                d->orgVSizes = sizes;
                sizes[2] += offset;
                sizes[1] -= offset;
                d->verticalSplitter->setSizes(sizes);
                break;
            }
            case Qt::LeftEdge: {
                auto sizes = d->horizontalSplitter->sizes();
                auto offset = size - sizes[0];
                sizes[0] += offset;
                sizes[1] -= offset;
                d->horizontalSplitter->setSizes(sizes);
                break;
            }
            case Qt::RightEdge: {
                auto sizes = d->horizontalSplitter->sizes();
                auto offset = size - sizes[2];
                sizes[2] += offset;
                sizes[1] -= offset;
                d->horizontalSplitter->setSizes(sizes);
                break;
            }
        }
    }

    QList<int> DockWidget::orientationSizes(Qt::Orientation orientation) const {
        Q_D(const DockWidget);
        switch (orientation) {
            case Qt::Horizontal: {
                auto sizes = d->horizontalSplitter->sizes();
                if (sizes == QList<int>{0, 0, 0}) {
                    sizes = {0, d->horizontalSplitter->width(), 0};
                }
                return sizes;
            }
            case Qt::Vertical: {
                auto sizes = d->verticalSplitter->sizes();
                if (sizes == QList<int>{0, 0, 0}) {
                    sizes = {0, d->verticalSplitter->height(), 0};
                }
                return sizes;
            }
        }
        return {};
    }

    void DockWidget::setOrientationSizes(Qt::Orientation orientation, const QList<int> &sizes) {
        Q_D(DockWidget);
        switch (orientation) {
            case Qt::Horizontal:
                d->horizontalSplitter->setSizes(sizes);
                break;
            case Qt::Vertical:
                d->verticalSplitter->setSizes(sizes);
                break;
        }
    }

    void DockWidget::toggleMaximize(Qt::Edge edge) {
        Q_D(DockWidget);
        switch (edge) {
            case Qt::TopEdge: {
                auto offset = d->horizontalSplitter->height() -
                              d->horizontalSplitter->minimumSizeHint().height();
                if (offset == 0) {
                    d->verticalSplitter->setSizes(d->orgVSizes);
                } else {
                    auto sizes = d->verticalSplitter->sizes();
                    d->orgVSizes = sizes;
                    sizes[0] += offset;
                    sizes[1] -= offset;
                    d->verticalSplitter->setSizes(sizes);
                }
                break;
            }
            case Qt::BottomEdge: {
                auto offset = d->horizontalSplitter->height() -
                              d->horizontalSplitter->minimumSizeHint().height();
                if (offset == 0) {
                    d->verticalSplitter->setSizes(d->orgVSizes);
                } else {
                    auto sizes = d->verticalSplitter->sizes();
                    d->orgVSizes = sizes;
                    sizes[2] += offset;
                    sizes[1] -= offset;
                    d->verticalSplitter->setSizes(sizes);
                }
                break;
            }
            case Qt::LeftEdge: {
                auto offset =
                    d->centralContainer->width() - d->centralContainer->minimumSizeHint().width();
                if (offset == 0) {
                    d->horizontalSplitter->setSizes(d->orgHSizes);
                } else {
                    auto sizes = d->horizontalSplitter->sizes();
                    d->orgHSizes = sizes;
                    sizes[0] += offset;
                    sizes[1] -= offset;
                    d->horizontalSplitter->setSizes(sizes);
                }
                break;
            }
            case Qt::RightEdge: {
                auto offset =
                    d->centralContainer->width() - d->centralContainer->minimumSizeHint().width();
                if (offset == 0) {
                    d->horizontalSplitter->setSizes(d->orgHSizes);
                } else {
                    auto sizes = d->horizontalSplitter->sizes();
                    d->orgHSizes = sizes;
                    sizes[2] += offset;
                    sizes[1] -= offset;
                    d->horizontalSplitter->setSizes(sizes);
                }
                break;
            }
        }
    }

    QWidget *DockWidget::findButton(const QWidget *w) const {
        Q_D(const DockWidget);
        return d->widgetIndexes.value(const_cast<QWidget *>(w));
    }

    bool DockWidget::barVisible(Qt::Edge edge) {
        Q_D(const DockWidget);
        return d->bars[edge2index(edge)]->isVisible();
    }

    void DockWidget::setBarVisible(Qt::Edge edge, bool visible) {
        Q_D(const DockWidget);
        d->bars[edge2index(edge)]->setVisible(visible);
    }

    bool DockWidget::dockAttribute(DockWidget::Attribute attr) {
        Q_D(const DockWidget);
        return d->attributes[attr];
    }

    void DockWidget::setDockAttribute(DockWidget::Attribute attr, bool on) {
        Q_D(DockWidget);
        d->attributes[attr] = on;
    }

    DockWidget::DockWidget(DockWidgetPrivate &d, DockButtonDelegate *delegate, QWidget *parent)
        : QFrame(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.delegate.reset(delegate);

        d.init();
    }

}