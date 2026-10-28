#include "sessionlockview.h"

#include "sessionlock.h"

#include <QPainter>
#include <QTimer>

SessionLockView::SessionLockView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::Image);
    setAntialiasing(false);
}

SessionLockView::~SessionLockView()
{
    m_view.setSurface(nullptr);
}

void SessionLockView::setLockSurface(SessionLockSurface *surface)
{
    if (m_lockSurface == surface)
        return;
    m_lockSurface = surface;
    attachSurface(surface);
    emit lockSurfaceChanged();
}

void SessionLockView::attachSurface(SessionLockSurface *surface)
{
    if (!surface || !surface->surface()) {
        m_view.setSurface(nullptr);
        update();
        return;
    }
    m_view.setSurface(surface->surface());
    connect(surface->surface(), &QWaylandSurface::damaged, this, [this] { update(); },
            Qt::UniqueConnection);
    QTimer *pulse = new QTimer(this);
    pulse->setInterval(120);
    connect(pulse, &QTimer::timeout, this, [this] {
        if (isVisible() && m_view.surface() && m_view.surface()->hasContent())
            update();
    });
    pulse->start();
    update();
}

void SessionLockView::paint(QPainter *painter)
{
    if (!m_view.surface()) {
        painter->fillRect(boundingRect(), QColor(0x10, 0x10, 0x14));
        return;
    }
    m_view.advance();
    const QImage frame = m_view.currentBuffer().image();
    if (!frame.isNull())
        m_lastFrame = frame;
    if (m_lastFrame.isNull()) {
        painter->fillRect(boundingRect(), QColor(0x10, 0x10, 0x14));
        return;
    }
    painter->drawImage(boundingRect(), m_lastFrame);
}
