#ifndef DOCKBUTTONDELEGATE_H
#define DOCKBUTTONDELEGATE_H

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QMenu>

#include <JetBrainsDockingSystem/jbdsnamespace.h>

namespace JBDS {

    class JBDS_EXPORT DockButtonDelegate {
    public:
        virtual ~DockButtonDelegate();

        virtual QAbstractButton *create(QWidget *parent) const = 0;

        virtual Qt::Orientation buttonOrientation(const QAbstractButton *button) const = 0;
        virtual void setButtonOrientation(QAbstractButton *button,
                                          Qt::Orientation orientation) const = 0;

        virtual QMenu *createViewModeMenu(QAbstractButton *button, QWidget *parent) const = 0;
    };

}

#endif // DOCKBUTTONDELEGATE_H
