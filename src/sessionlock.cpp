#include "sessionlock.h"

#include <QMetaObject>

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSeat>

SessionLockIntegration::SessionLockIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration(item)
    , m_item(item)
{
    if (auto *surf = qobject_cast<SessionLockSurface *>(m_item->shellSurface()))
        m_item->setSurface(surf->surface());
}

QWaylandSurfaceRole SessionLockSurface::s_role_impl("ext_session_lock_surface_v1");

QWaylandSurfaceRole *SessionLockSurface::role()
{
    return &s_role_impl;
}

SessionLockSurface::SessionLockSurface(SessionLock *lock, QWaylandSurface *surface,
                                       QWaylandOutput *output, const QWaylandResource &resource)
    : m_surface(surface)
    , m_output(output)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();

    QMetaObject::invokeMethod(this, [this] { sendConfigure(); }, Qt::QueuedConnection);

}

SessionLockSurface::~SessionLockSurface() = default;

SessionLockSurface *SessionLockSurface::fromResource(::wl_resource *resource)
{
    auto *res = Resource::fromResource(resource);
    return res ? static_cast<SessionLockSurface *>(res->object()) : nullptr;
}

QWaylandQuickShellIntegration *SessionLockSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new SessionLockIntegration(item);
}

void SessionLockSurface::sendConfigure()
{
    if (!m_surface)
        return;
    const QSize size = m_output ? m_output->geometry().size() : QSize(1280, 800);
    m_pendingSerial = m_surface->compositor()->nextSerial();
    send_configure(m_pendingSerial, uint32_t(size.width()), uint32_t(size.height()));
}

void SessionLockSurface::ext_session_lock_surface_v1_ack_configure(Resource *resource, uint32_t serial)
{
    if (serial != m_pendingSerial) {
        wl_resource_post_error(resource->handle, EXT_SESSION_LOCK_SURFACE_V1_ERROR_INVALID_SERIAL,
                               "unexpected ack_configure serial %u", serial);
        return;
    }
    m_ackedInitial = true;
}

void SessionLockSurface::ext_session_lock_surface_v1_destroy(Resource *)
{
}

void SessionLockSurface::ext_session_lock_surface_v1_destroy_resource(Resource *)
{
    emit surfaceGone(this);
    delete this;
}

SessionLock::SessionLock(SessionLockManager *manager, const QWaylandResource &resource)
    : m_manager(manager)
{
    init(resource.resource());
}

SessionLock::~SessionLock() = default;

SessionLock *SessionLock::fromResource(::wl_resource *resource)
{
    auto *res = Resource::fromResource(resource);
    return res ? static_cast<SessionLock *>(res->object()) : nullptr;
}

bool SessionLock::hasSurfaceFor(QWaylandOutput *output) const
{
    for (SessionLockSurface *s : m_surfaces) {
        if (s && s->output() == output)
            return true;
    }
    return false;
}

void SessionLock::finish()
{
    if (m_released)
        return;
    m_released = true;
    if (m_locked) {
        m_locked = false;
        emit lockReleased(this);
    }
    releaseSurfaces();
}

void SessionLock::releaseSurfaces()
{
    const QList<SessionLockSurface *> surfaces = m_surfaces;
    m_surfaces.clear();
    for (SessionLockSurface *s : surfaces) {
        if (!s)
            continue;
        emit surfaceRemoved(s);
        auto *res = s->resource();
        if (res && res->handle)
            wl_resource_destroy(res->handle);
        else
            delete s;
    }
}

void SessionLock::ext_session_lock_v1_get_lock_surface(Resource *resource, uint32_t id,
                                                       ::wl_resource *surface,
                                                       ::wl_resource *output)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (!qs) {
        wl_resource_post_error(resource->handle, EXT_SESSION_LOCK_V1_ERROR_ROLE,
                               "invalid wl_surface");
        return;
    }
    QWaylandOutput *out = output ? QWaylandOutput::fromResource(output) : nullptr;
    if (!out) {
        auto *manager = m_manager.data();
        auto *compositor = manager
            ? qobject_cast<QWaylandCompositor *>(manager->extensionContainer())
            : nullptr;
        out = compositor ? compositor->defaultOutput() : nullptr;
    }
    if (hasSurfaceFor(out)) {
        wl_resource_post_error(resource->handle, EXT_SESSION_LOCK_V1_ERROR_DUPLICATE_OUTPUT,
                               "lock surface already exists for output");
        return;
    }
    if (!qs->setRole(SessionLockSurface::role(), resource->handle, EXT_SESSION_LOCK_V1_ERROR_ROLE))
        return;

    QWaylandResource surfResource(wl_resource_create(resource->client(),
                                                     &::ext_session_lock_surface_v1_interface,
                                                     wl_resource_get_version(resource->handle), id));
    if (!surfResource.resource())
        return;

    SessionLockSurface *ls = new SessionLockSurface(this, qs, out, surfResource);
    m_surfaces.append(ls);
    connect(ls, &SessionLockSurface::surfaceGone, this, [this, ls](SessionLockSurface *s) {
        Q_UNUSED(s)
        emit surfaceRemoved(ls);
        m_surfaces.removeAll(ls);
    });
    connect(qs, &QWaylandSurface::hasContentChanged, ls, [this, qs] {
        if (qs->hasContent() && !m_locked && !m_released) {
            m_locked = true;
            emit lockActivated(this);
        }
    });
    emit surfaceAdded(ls);
}

void SessionLock::ext_session_lock_v1_unlock_and_destroy(Resource *resource)
{
    if (!m_locked && !m_surfaces.isEmpty()) {
        wl_resource_post_error(resource->handle, EXT_SESSION_LOCK_V1_ERROR_INVALID_UNLOCK,
                               "unlock_and_destroy before locked");
        return;
    }
    finish();
    wl_resource_destroy(resource->handle);
}

void SessionLock::ext_session_lock_v1_destroy(Resource *resource)
{
    if (m_locked) {
        wl_resource_post_error(resource->handle, EXT_SESSION_LOCK_V1_ERROR_INVALID_DESTROY,
                               "destroy while locked, use unlock_and_destroy");
        return;
    }
    finish();
    wl_resource_destroy(resource->handle);
}

void SessionLock::ext_session_lock_v1_destroy_resource(Resource *)
{
    deleteLater();
}

SessionLockManager::SessionLockManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<SessionLockManager>()
{
    Q_UNUSED(parent)
}

SessionLockManager::~SessionLockManager() = default;

void SessionLockManager::initialize()
{
    QWaylandCompositorExtensionTemplate<SessionLockManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    qInfo() << "SessionLock: ext_session_lock_v1 ready";
}

void SessionLockManager::setLocked(bool locked)
{
    if (m_locked == locked)
        return;
    m_locked = locked;
    emit lockedChanged();
}

void SessionLockManager::addSurface(SessionLockSurface *surface)
{
    if (!surface)
        return;
    m_lockSurfaces.append(QVariant::fromValue(surface));
    emit lockSurfacesChanged();
}

void SessionLockManager::removeSurface(SessionLockSurface *surface)
{
    if (!surface)
        return;
    m_lockSurfaces.removeAll(QVariant::fromValue(surface));
    emit lockSurfacesChanged();
}

void SessionLockManager::ext_session_lock_manager_v1_destroy(Resource *)
{
}

void SessionLockManager::ext_session_lock_manager_v1_lock(Resource *resource, uint32_t id)
{
    if (m_activeLock) {
        QWaylandResource finishedResource(wl_resource_create(
            resource->client(), &::ext_session_lock_v1_interface,
            wl_resource_get_version(resource->handle), id));
        if (!finishedResource.resource())
            return;
        SessionLock *doomed = new SessionLock(this, finishedResource);
        doomed->send_finished();
        return;
    }
    QWaylandResource lockResource(wl_resource_create(resource->client(),
                                                     &::ext_session_lock_v1_interface,
                                                     wl_resource_get_version(resource->handle), id));
    if (!lockResource.resource())
        return;
    SessionLock *lock = new SessionLock(this, lockResource);
    m_activeLock = lock;
    connect(lock, &SessionLock::lockActivated, this, [this](SessionLock *l) {
        if (l == m_activeLock)
            setLocked(true);
        auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
        if (compositor && compositor->defaultSeat()) {
            const QVariantList surfaces = m_lockSurfaces;
            for (const QVariant &v : surfaces) {
                auto *s = v.value<SessionLockSurface *>();
                if (s && s->surface()) {
                    compositor->defaultSeat()->setKeyboardFocus(s->surface());
                    break;
                }
            }
        }
        l->send_locked();
    });
    connect(lock, &SessionLock::lockReleased, this, [this](SessionLock *l) {
        if (l == m_activeLock) {
            m_activeLock = nullptr;
            setLocked(false);
        }
        auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
        if (compositor && compositor->defaultSeat())
            compositor->defaultSeat()->setKeyboardFocus(nullptr);
    });
    connect(lock, &SessionLock::surfaceAdded, this, &SessionLockManager::addSurface);
    connect(lock, &SessionLock::surfaceRemoved, this, &SessionLockManager::removeSurface);
}
