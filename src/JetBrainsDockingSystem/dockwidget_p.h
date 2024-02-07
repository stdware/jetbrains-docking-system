#ifndef DOCKWIDGET_P_H
#define DOCKWIDGET_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the JetBrainsDockingSystem API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtCore/QSet>
#include <QtCore/QHash>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QStackedWidget>

#include <JetBrainsDockingSystem/dockwidget.h>
#include <JetBrainsDockingSystem/dockpanel_p.h>
#include <JetBrainsDockingSystem/docksidebar_p.h>
#include <JetBrainsDockingSystem/dockdragcontroller_p.h>

namespace JBDS {

    struct DockButtonData {
        ViewMode viewMode;
        Qt::Edge edge;
        Side side;
        QWidget *widget;
        QWidget *container;
        QObject *floatingHelper;
        QObject *widgetEventFilter;
        QObject *buttonEventFilter;
    };

    class DockWidgetPrivate : public QObject {
        Q_DECLARE_PUBLIC(DockWidget)
    public:
        DockWidgetPrivate();
        virtual ~DockWidgetPrivate();

        void init();

        DockWidget *q_ptr;

        QScopedPointer<DockButtonDelegate> delegate;

        int resizeMargin = 8;

        // Modules
        DockPanel *panels[4];
        DockSideBar *bars[4];
        QSplitter *splitters[2];
        QStackedWidget *centralContainer;
        QGridLayout *mainLayout;

        QScopedPointer<DockDragController> dragCtl;

        QHash<QAbstractButton *, DockButtonData> buttonDataHash;
        QHash<QWidget *, QAbstractButton *> widgetIndexes;

        void barButtonAdded(Qt::Edge edge, Side side, QAbstractButton *button);
        void barButtonRemoved(Qt::Edge edge, Side side, QAbstractButton *button);
        void barButtonViewModeChanged(Qt::Edge edge, Side side, QAbstractButton *button,
                                      ViewMode oldViewMode);

        static inline DockWidgetPrivate *get(DockWidget *q) {
            return q->d_func();
        }

        static inline const DockWidgetPrivate *get(const DockWidget *q) {
            return q->d_func();
        }

        inline bool dockVisible(const QAbstractButton *button) {
            return button->isChecked() &&
                   buttonDataHash.value(const_cast<QAbstractButton *>(button)).viewMode ==
                       DockPinned;
        }

    private:
        void _q_widgetDestroyed();
        void _q_buttonDestroyed();
        void _q_buttonToggled(bool checked);
    };

    inline int edge2index(Qt::Edge e) {
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

}

#endif // DOCKWIDGET_P_H
