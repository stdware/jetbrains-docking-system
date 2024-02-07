#ifndef QMFLOATINGWINDOWHELPER_H
#define QMFLOATINGWINDOWHELPER_H

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the JetBrainsDockingSystem API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <QMargins>
#include <QObject>

class QMFloatingWindowHelperPrivate;

class QMFloatingWindowHelper : public QObject {
    Q_OBJECT
public:
    explicit QMFloatingWindowHelper(QWidget *w, QObject *parent = nullptr);
    ~QMFloatingWindowHelper();

public:
    bool floating() const;
    void setFloating(bool floating, Qt::WindowFlags flags = Qt::Window);

    QMargins resizeMargins() const;
    void setResizeMargins(const QMargins &resizeMargins);

private:
    QMFloatingWindowHelperPrivate *d;
};

#endif // QMFLOATINGWINDOWHELPER_H
