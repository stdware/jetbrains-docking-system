#ifndef DOCKDRAGCONTROLLER_P_H
#define DOCKDRAGCONTROLLER_P_H

#include <QtWidgets/QLabel>

#include <JetBrainsDockingSystem/dockwidget.h>
#include <JetBrainsDockingSystem/docksidebar_p.h>

namespace JBDS {

    class DockDragController : public QObject {
    public:
        explicit DockDragController(DockWidget *dock, QObject *parent = nullptr);
        ~DockDragController();

    public:
        void startDrag(QAbstractButton *button, const QPoint &pos, const QPixmap &pixmap);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

        DockWidget *m_dock;
        QWidget *m_label;

    private:
        void tabDragMove();
        void tabDragOver();
    };

}

#endif // DOCKDRAGCONTROLLER_P_H
