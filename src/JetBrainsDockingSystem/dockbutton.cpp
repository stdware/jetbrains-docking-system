#include "dockbutton.h"
#include "dockbutton_p.h"

#include <QMenu>

namespace JBDS {

    QAbstractButton *DefaultDockButtonDelegate::create(QWidget *parent) const {
        return new DockButton(parent);
    }

    Qt::Orientation
        DefaultDockButtonDelegate::buttonOrientation(const QAbstractButton *button) const {
        return static_cast<const DockButton *>(button)->orientation();
    }

    void DefaultDockButtonDelegate::setButtonOrientation(QAbstractButton *button,
                                                         Qt::Orientation orientation) const {
        static_cast<DockButton *>(button)->setOrientation(orientation);
    }

    QMenu *DefaultDockButtonDelegate::createViewModeMenu(QAbstractButton *button,
                                                         QWidget *parent) const {
        return new QMenu(parent);
    }

    DockButtonPrivate::DockButtonPrivate() {
    }

    DockButtonPrivate::~DockButtonPrivate() {
    }

    void DockButtonPrivate::init() {
    }

    DockButton::DockButton(QWidget *parent) : DockButton(*new DockButtonPrivate(), parent) {
    }

    DockButton::~DockButton() {
    }

    Qt::Orientation DockButton::orientation() const {
        Q_D(const DockButton);
        return d->orientation;
    }

    void DockButton::setOrientation(Qt::Orientation orientation) {
        Q_D(DockButton);
        d->orientation = orientation;
    }

    DockButton::DockButton(DockButtonPrivate &d, QWidget *parent) : QPushButton(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
