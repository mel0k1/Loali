#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QVariantList>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandQuickShellIntegration>
#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandShellSurface>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandSurfaceRole>

#include "qwayland-server-ext-session-lock-v1.h"

class QWaylandCompositor;
class SessionLock;
class SessionLockManager;

class SessionLockIntegration : public QWaylandQuickShellIntegration
{
public:
    explicit SessionLockIntegration(QWaylandQuickShellSurfaceItem *item);

private:
    QWaylandQuickShellSurfaceItem *m_item = nullptr;
};

class SessionLockSurface : public QWaylandShellSurfaceTemplate<SessionLockSurface>,
                           public QtWaylandServer::ext_session_lock_surface_v1
{
    Q_OBJECT
    Q_PROPERTY(QWaylandSurface *surface READ surface CONSTANT)
public:
    SessionLockSurface(SessionLock *lock, QWaylandSurface *surface, QWaylandOutput *output,
                       const QWaylandResource &resource);
    ~SessionLockSurface() override;

    using QtWaylandServer::ext_session_lock_surface_v1::interface;
    using QtWaylandServer::ext_session_lock_surface_v1::interfaceName;

    static QWaylandSurfaceRole *role();
    static SessionLockSurface *fromResource(::wl_resource *resource);

    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override;

    QWaylandSurface *surface() const { return m_surface.data(); }
    QWaylandOutput *output() const { return m_output.data(); }

    void sendConfigure();

signals:
    void surfaceGone(SessionLockSurface *surface);

protected:
    void ext_session_lock_surface_v1_ack_configure(Resource *resource, uint32_t serial) override;
    void ext_session_lock_surface_v1_destroy(Resource *resource) override;
    void ext_session_lock_surface_v1_destroy_resource(Resource *resource) override;

private:
    static QWaylandSurfaceRole s_role_impl;

    QPointer<QWaylandSurface> m_surface;
    QPointer<QWaylandOutput> m_output;
    uint32_t m_pendingSerial = 0;
    bool m_ackedInitial = false;
};

class SessionLock : public QObject, public QtWaylandServer::ext_session_lock_v1
{
    Q_OBJECT
public:
    SessionLock(SessionLockManager *manager, const QWaylandResource &resource);
    ~SessionLock() override;

    using QtWaylandServer::ext_session_lock_v1::interface;
    using QtWaylandServer::ext_session_lock_v1::interfaceName;

    static SessionLock *fromResource(::wl_resource *resource);

    bool isLocked() const { return m_locked; }
    bool hasSurfaceFor(QWaylandOutput *output) const;
    void finish();

signals:
    void lockActivated(SessionLock *lock);
    void lockReleased(SessionLock *lock);
    void surfaceAdded(SessionLockSurface *surface);
    void surfaceRemoved(SessionLockSurface *surface);

protected:
    void ext_session_lock_v1_get_lock_surface(Resource *resource, uint32_t id,
                                              ::wl_resource *surface, ::wl_resource *output) override;
    void ext_session_lock_v1_unlock_and_destroy(Resource *resource) override;
    void ext_session_lock_v1_destroy(Resource *resource) override;
    void ext_session_lock_v1_destroy_resource(Resource *resource) override;

private:
    void releaseSurfaces();

    QPointer<SessionLockManager> m_manager;
    QList<SessionLockSurface *> m_surfaces;
    bool m_locked = false;
    bool m_released = false;
};

class SessionLockManager : public QWaylandCompositorExtensionTemplate<SessionLockManager>,
                           public QtWaylandServer::ext_session_lock_manager_v1
{
    Q_OBJECT
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)
    Q_PROPERTY(QVariantList lockSurfaces READ lockSurfaces NOTIFY lockSurfacesChanged)

public:
    explicit SessionLockManager(QObject *parent = nullptr);
    ~SessionLockManager() override;
    void initialize() override;

    using QtWaylandServer::ext_session_lock_manager_v1::interface;
    using QtWaylandServer::ext_session_lock_manager_v1::interfaceName;

    bool locked() const { return m_activeLock && m_activeLock->isLocked(); }
    QVariantList lockSurfaces() const { return m_lockSurfaces; }

signals:
    void lockedChanged();
    void lockSurfacesChanged();

protected:
    void ext_session_lock_manager_v1_lock(Resource *resource, uint32_t id) override;
    void ext_session_lock_manager_v1_destroy(Resource *resource) override;

private:
    void setLocked(bool locked);
    void addSurface(SessionLockSurface *surface);
    void removeSurface(SessionLockSurface *surface);

    SessionLock *m_activeLock = nullptr;
    QVariantList m_lockSurfaces;
    bool m_locked = false;

    friend class SessionLock;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(SessionLockManager)
