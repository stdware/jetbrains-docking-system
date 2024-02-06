#ifndef DOCKPANEL_P_H
#define DOCKPANEL_P_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the JetBrainsDockingSystem API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>

#include <JetBrainsDockingSystem/dockwidget.h>

namespace JBDS {

    class DockPanel : public QSplitter {
        Q_OBJECT
    public:
        explicit DockPanel(Qt::Orientation orient, QWidget *parent = nullptr);
        ~DockPanel();

    public:
        void addWidget(QWidget *widget) = delete;
        void insertWidget(int index, QWidget *widget) = delete;
        QWidget *replaceWidget(int index, QWidget *widget) = delete;

        int addWidget(Side side, QWidget *w, bool visible = true);
        int insertWidget(Side side, int index, QWidget *w, bool visible = true);
        void removeWidget(Side side, QWidget *w);

        QWidget *currentWidget(Side side) const;
        int currentIndex(Side side) const;

        int indexOf(Side side, QWidget *w) const;
        QWidget *widget(Side side, int index) const;
        int count(Side side) const;

        void setCurrentIndex(Side side, int index);
        void setCurrentWidget(Side side, QWidget *w);

        void setContainerVisible(Side side, bool visible);

    protected:
        QStackedWidget *m_firstWidget;
        QStackedWidget *m_secondWidget;

    private:
        void updateVisibility();
    };

}

#endif // DOCKPANEL_P_H
