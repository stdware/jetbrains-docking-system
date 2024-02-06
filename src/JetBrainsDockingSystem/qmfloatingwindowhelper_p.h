#ifndef QMFLOATINGWINDOWHELPER_H
#define QMFLOATINGWINDOWHELPER_H

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
