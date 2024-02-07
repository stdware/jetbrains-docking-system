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

        QWidget *findButton(const QWidget *w) const;

        bool barVisible(Qt::Edge edge);
        void setBarVisible(Qt::Edge edge, bool visible);

    protected:
        DockWidget(DockWidgetPrivate &d, DockButtonDelegate *delegate, QWidget *parent = nullptr);

        QScopedPointer<DockWidgetPrivate> d_ptr;
    };

    inline QAbstractButton *DockWidget::addWidget(Qt::Edge edge, Side side, QWidget *w) {
        return insertWidget(edge, side, -1, w);
    }

}

#endif // DOCKWIDGET_H
