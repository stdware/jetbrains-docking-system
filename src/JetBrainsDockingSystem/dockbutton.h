#ifndef DOCKBUTTON_H
#define DOCKBUTTON_H

#include <QtWidgets/QPushButton>

#include <JetBrainsDockingSystem/dockbuttondelegate.h>

namespace JBDS {

    class JBDS_EXPORT DefaultDockButtonDelegate : public DockButtonDelegate {
    public:
        QAbstractButton *create(QWidget *parent) const override;

        Qt::Orientation buttonOrientation(const QAbstractButton *button) const override;
        void setButtonOrientation(QAbstractButton *button,
                                  Qt::Orientation orientation) const override;

        QMenu *createViewModeMenu(QAbstractButton *button, QWidget *parent) const override;
    };

    class DockButtonPrivate;

    class JBDS_EXPORT DockButton : public QPushButton {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DockButton)
    public:
        explicit DockButton(QWidget *parent = nullptr);
        explicit DockButton(const QString &text, QWidget *parent = nullptr);
        DockButton(const QIcon &icon, const QString &text, QWidget *parent = nullptr);
        ~DockButton();

    public:
        Qt::Orientation orientation() const;
        void setOrientation(Qt::Orientation orientation);

    protected:
        DockButton(DockButtonPrivate &d, QWidget *parent = nullptr);

        QScopedPointer<DockButtonPrivate> d_ptr;
    };

}

#endif // DOCKBUTTON_H
