#include "dockwidget.h"
#include "dockwidget_p.h"

#include <QtCore/QTimer>
#include <QtGui/QtEvents>
#include <QtGui/QWindow>
#include <QtGui/QPixmap>
#include <QtGui/QGuiApplication>

#include <private/qapplication_p.h>

#include "dockbutton.h"

#include "qmfloatingwindowhelper_p.h"

namespace JBDS {

    // NOTE: Remember to enable QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps)
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
        QPixmap pixmap = createPixmap(button->size(), button->window()->windowHandle());
        pixmap.fill(Qt::transparent);
        button->render(&pixmap);
        return pixmap;
    }

    static void removeDockButtonData(const DockButtonData &data) {
        data.container->deleteLater();
        delete data.floatingHelper;
        delete data.widgetEventFilter;
        delete data.buttonEventFilter;
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
                QApplicationPrivate::active_window = m_org;
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
        WidgetEventFilter(DockWidgetPrivate *d, QWidget *w, QAbstractButton *button)
            : d(d), widget(w), button(button), closing(false) {
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
                    auto org = QApplicationPrivate::active_window;
                    QApplicationPrivate::active_window = button->window();

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
                        QApplicationPrivate::active_window = org;
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
        ButtonEventFilter(DockWidgetPrivate *d, QWidget *w, QAbstractButton *button)
            : d(d), widget(w), button(button), readyDrag(false) {
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

                    d->dragCtl->startDrag(button, dragPos, buttonShot(button));
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

        bool eventFilter(QObject *obj, QEvent *event) override {
            switch (event->type()) {
                case QEvent::MouseButtonPress:
                case QEvent::MouseMove:
                case QEvent::MouseButtonRelease:
                    mousePressEvent(static_cast<QMouseEvent *>(event));
                    break;

                case QEvent::Leave:
                    leaveEvent(event);
                    break;

                default:
                    break;
            }
            return QObject::eventFilter(obj, event);
        }
    };

    static inline int edge2index(Qt::Edge e) {
        int res = 0;
        switch (e) {
            case Qt::TopEdge:
                res = 1;
                break;
            case Qt::RightEdge:
                res = 2;
                break;
            case Qt::BottomEdge:
                res = 3;
                break;
            default:
                break;
        }
        return res;
    }

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

        splitters[0] = new QSplitter(Qt::Horizontal);
        splitters[0]->setChildrenCollapsible(false);
        splitters[0]->addWidget(panels[0]);
        splitters[0]->addWidget(centralContainer);
        splitters[0]->addWidget(panels[2]);

        splitters[0]->setStretchFactor(0, 0);
        splitters[0]->setStretchFactor(1, 1);
        splitters[0]->setStretchFactor(2, 0);

        splitters[1] = new QSplitter(Qt::Vertical);
        splitters[1]->setChildrenCollapsible(false);
        splitters[1]->addWidget(panels[1]);
        splitters[1]->addWidget(splitters[0]);
        splitters[1]->addWidget(panels[3]);

        splitters[1]->setStretchFactor(0, 0);
        splitters[1]->setStretchFactor(1, 1);
        splitters[1]->setStretchFactor(2, 0);

        mainLayout->addWidget(bars[0], 1, 0);
        mainLayout->addWidget(bars[1], 0, 1);
        mainLayout->addWidget(bars[2], 1, 2);
        mainLayout->addWidget(bars[3], 2, 1);
        mainLayout->addWidget(splitters[1], 1, 1);

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

    void DockWidgetPrivate::barButtonViewModeChanged(Qt::Edge edge, Side side,
                                                     QAbstractButton *button,
                                                     ViewMode oldViewMode) {
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
        bars[edge2index(data.edge)]->buttonToggled(data.side, button);
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

    QAbstractButton *DockWidget::addWidget(Qt::Edge edge, Side side, QWidget *w) {
        Q_D(DockWidget);
        if (d->widgetIndexes.contains(w))
            return nullptr;

        auto button = d->delegate->create(nullptr);
        QWidget *container;
        {
            container = new QWidget();
            container->setObjectName("dock_widget_container");
            container->setAttribute(Qt::WA_StyledBackground);

            auto layout = new QVBoxLayout();
            layout->setContentsMargins({});
            layout->setSpacing(0);

            layout->addWidget(w);
            w->installEventFilter(this);

            container->setLayout(layout);
        }

        auto floatingHelper = new QMFloatingWindowHelper(w, this);
        floatingHelper->setResizeMargins(
            {d->resizeMargin, d->resizeMargin, d->resizeMargin, d->resizeMargin});

        DockButtonData data{
            DockPinned,
            edge,
            side,
            w,
            container,
            floatingHelper,
            new WidgetEventFilter(d, w, button),
            new ButtonEventFilter(d, w, button),
        };
        d->buttonDataHash.insert(button, data);
        d->widgetIndexes.insert(w, button);
        connect(w, &QObject::destroyed, d, &DockWidgetPrivate::_q_widgetDestroyed);
        connect(button, &QObject::destroyed, d, &DockWidgetPrivate::_q_buttonDestroyed);
        connect(button, &QAbstractButton::toggled, d, &DockWidgetPrivate::_q_buttonToggled);
        return button;
    }

    void DockWidget::removeWidget(QAbstractButton *button) {
        Q_D(DockWidget);

        auto it = d->buttonDataHash.find(button);
        if (it == d->buttonDataHash.end())
            return;

        auto &data = it.value();
        if (QObject *o = data.widget; qobject_cast<QWidget *>(o)) {
            data.widget->setParent(nullptr);
        }
        d->bars[edge2index(data.edge)]->removeButton(data.side, button);
        removeDockButtonData(data);

        disconnect(data.widget, &QObject::destroyed, d, &DockWidgetPrivate::_q_widgetDestroyed);
        disconnect(button, &QObject::destroyed, d, &DockWidgetPrivate::_q_buttonDestroyed);
        disconnect(button, &QAbstractButton::toggled, d, &DockWidgetPrivate::_q_buttonToggled);

        d->buttonDataHash.erase(it);
        d->widgetIndexes.remove(data.widget);
    }

    void DockWidget::moveWidget(QAbstractButton *button, Qt::Edge edge, Side number) {
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

    DockWidget::DockWidget(DockWidgetPrivate &d, DockButtonDelegate *delegate, QWidget *parent)
        : QFrame(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.delegate.reset(delegate);

        d.init();
    }

}