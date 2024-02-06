#include "dockbutton.h"
#include "dockbutton_p.h"

#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleOptionButton>
#include <QtWidgets/QStylePainter>

namespace JBDS {

    QAbstractButton *DefaultDockButtonDelegate::create(QWidget *parent) const {
        return new DockButton(parent);
    }

    Orientation DefaultDockButtonDelegate::buttonOrientation(const QAbstractButton *button) const {
        return static_cast<const DockButton *>(button)->orientation();
    }

    void DefaultDockButtonDelegate::setButtonOrientation(QAbstractButton *button,
                                                         Orientation orientation) const {
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

    DockButton::DockButton(const QString &text, QWidget *parent) : DockButton(parent) {
        setText(text);
    }

    DockButton::DockButton(const QIcon &icon, const QString &text, QWidget *parent)
        : DockButton(text, parent) {
        setIcon(icon);
    }

    DockButton::~DockButton() {
    }

    QSize DockButton::sizeHint() const {
        Q_D(const DockButton);
        QSize sz = QPushButton::sizeHint();
        if (d->orientation != Horizontal) {
            sz.transpose();
        }
        return sz;
    }

    Orientation DockButton::orientation() const {
        Q_D(const DockButton);
        return d->orientation;
    }

    void DockButton::setOrientation(Orientation orientation) {
        Q_D(DockButton);
        d->orientation = orientation;
        updateGeometry();
    }

    void DockButton::paintEvent(QPaintEvent *event) {
        Q_D(DockButton);

        Q_UNUSED(event);

        QStylePainter painter(this);
        QStyleOptionButton option;
        initStyleOption(&option);

        if (d->orientation == TopToBottom) {
            painter.rotate(90);
            painter.translate(0, -1 * width());
            option.rect = option.rect.transposed();
        } else if (d->orientation == BottomToTop) {
            painter.rotate(-90);
            painter.translate(-1 * height(), 0);
            option.rect = option.rect.transposed();
        }
        painter.drawControl(QStyle::CE_PushButton, option);
    }

    DockButton::DockButton(DockButtonPrivate &d, QWidget *parent) : QPushButton(parent), d_ptr(&d) {
        d.q_ptr = this;

        d.init();
    }

}
