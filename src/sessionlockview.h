#pragma once

#include <QImage>
#include <QQuickPaintedItem>
#include <QtWaylandCompositor/QWaylandView>

class SessionLockSurface;

class SessionLockView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(SessionLockSurface *lockSurface READ lockSurface WRITE setLockSurface
                   NOTIFY lockSurfaceChanged)

public:
    explicit SessionLockView(QQuickItem *parent = nullptr);
    ~SessionLockView() override;

    SessionLockSurface *lockSurface() const { return m_lockSurface; }
    void setLockSurface(SessionLockSurface *surface);

    void paint(QPainter *painter) override;

signals:
    void lockSurfaceChanged();

private:
    void attachSurface(SessionLockSurface *surface);

    QWaylandView m_view;
    SessionLockSurface *m_lockSurface = nullptr;
    QImage m_lastFrame;
};
