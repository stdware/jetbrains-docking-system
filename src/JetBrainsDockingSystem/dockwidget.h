#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QtWidgets/QFrame>

#include <JetBrainsDockingSystem/dockbuttondelegate.h>

namespace JBDS {

    class DockWidgetPrivate;

    class JBDS_EXPORT DockWidget : public QFrame {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DockWidget)
        Q_PROPERTY(int resizeMargin READ resizeMargin WRITE setResizeMargin)
    public:
        explicit DockWidget(QWidget *parent = nullptr);
        DockWidget(DockButtonDelegate *delegate, QWidget *parent = nullptr);
        ~DockWidget();

        enum Attribute {
            ViewModeContextMenu,
            AutoFloatDraggingOutside,
        };

    public:
        int resizeMargin() const;
        void setResizeMargin(int resizeMargin);

        QWidget *widget() const;
        void setWidget(QWidget *w);
        QWidget *takeWidget();

        inline QAbstractButton *addWidget(Qt::Edge edge, Side side, QWidget *w);
        QAbstractButton *insertWidget(Qt::Edge edge, Side side, int index, QWidget *w);
        void removeWidget(QAbstractButton *button);
        void moveWidget(QAbstractButton *button, Qt::Edge edge, Side side, int index = -1);
        int widgetCount(Qt::Edge edge, Side side) const;
        QList<QWidget *> widgets(Qt::Edge edge, Side side) const;

        QWidget *widget(const QAbstractButton *button);
        ViewMode viewMode(const QAbstractButton *button);
        void setViewMode(QAbstractButton *button, ViewMode viewMode);

        int edgeSize(Qt::Edge edge) const;
        void setEdgeSize(Qt::Edge edge, int size);
        QList<int> orientationSizes(Qt::Orientation orientation) const;
        void setOrientationSizes(Qt::Orientation orientation, const QList<int> &sizes);
        void toggleMaximize(Qt::Edge edge);

        QWidget *findButton(const QWidget *w) const;

        bool barVisible(Qt::Edge edge);
        void setBarVisible(Qt::Edge edge, bool visible);

        bool dockAttribute(Attribute attr);
        void setDockAttribute(Attribute attr, bool on = true);

    protected:
        DockWidget(DockWidgetPrivate &d, DockButtonDelegate *delegate, QWidget *parent = nullptr);

        QScopedPointer<DockWidgetPrivate> d_ptr;
    };

    inline QAbstractButton *DockWidget::addWidget(Qt::Edge edge, Side side, QWidget *w) {
        return insertWidget(edge, side, -1, w);
    }

}

#endif // DOCKWIDGET_H
