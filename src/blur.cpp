#include "blur.h"

#include <QtWaylandCompositor/QWaylandCompositor>

BlurRegistry *BlurRegistry::s_instance = nullptr;

KdeBlur::KdeBlur(QWaylandSurface *surface, const QWaylandResource &resource)
    : m_surface(surface)
{
    init(resource.resource());
}

KdeBlur::~KdeBlur() = default;

void KdeBlur::org_kde_kwin_blur_set_region(Resource *, struct ::wl_resource *region)
{
    Q_UNUSED(region)
}

void KdeBlur::org_kde_kwin_blur_commit(Resource *)
{
    if (m_surface)
        BlurRegistry::instance()->setBlurred(m_surface, true);
}

void KdeBlur::org_kde_kwin_blur_release(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void KdeBlur::org_kde_kwin_blur_destroy_resource(Resource *)
{
    if (m_surface)
        BlurRegistry::instance()->setBlurred(m_surface, false);
    deleteLater();
}

KdeBlurManager::KdeBlurManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<KdeBlurManager>(qobject_cast<QWaylandObject *>(parent))
{
}

KdeBlurManager::~KdeBlurManager() = default;

void KdeBlurManager::initialize()
{
    QWaylandCompositorExtensionTemplate<KdeBlurManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    qInfo() << "Blur: org_kde_kwin_blur ready";
}

void KdeBlurManager::org_kde_kwin_blur_manager_create(Resource *resource, uint32_t id,
                                                      struct ::wl_resource *surface)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (!qs) {
        wl_resource_post_error(resource->handle, 0, "invalid surface");
        return;
    }
    QWaylandResource blurResource(wl_resource_create(resource->client(),
                                                     &::org_kde_kwin_blur_interface,
                                                     wl_resource_get_version(resource->handle),
                                                     id));
    if (!blurResource.resource())
        return;
    auto *blur = new KdeBlur(qs, blurResource);
    m_blurs.append(blur);
    connect(blur, &QObject::destroyed, this, [this, blur] { m_blurs.removeOne(blur); });
}

void KdeBlurManager::org_kde_kwin_blur_manager_unset(Resource *, struct ::wl_resource *surface)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (!qs)
        return;
    for (KdeBlur *blur : std::as_const(m_blurs)) {
        if (blur->m_surface == qs) {
            BlurRegistry::instance()->setBlurred(qs, false);
            blur->m_surface.clear();
            wl_resource_destroy(blur->resource()->handle);
            break;
        }
    }
}

void KdeBlurManager::org_kde_kwin_blur_manager_destroy_resource(Resource *)
{
}

ExtBackgroundEffectSurface::ExtBackgroundEffectSurface(QWaylandSurface *surface,
                                                       const QWaylandResource &resource)
    : m_surface(surface)
{
    init(resource.resource());
}

ExtBackgroundEffectSurface::~ExtBackgroundEffectSurface() = default;

void ExtBackgroundEffectSurface::ext_background_effect_surface_v1_set_blur_region(
    Resource *, struct ::wl_resource *region)
{
    if (m_surface)
        BlurRegistry::instance()->setBlurred(m_surface, region != nullptr);
}

void ExtBackgroundEffectSurface::ext_background_effect_surface_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void ExtBackgroundEffectSurface::ext_background_effect_surface_v1_destroy_resource(Resource *)
{
    if (m_surface)
        BlurRegistry::instance()->setBlurred(m_surface, false);
    deleteLater();
}

ExtBackgroundEffectManager::ExtBackgroundEffectManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<ExtBackgroundEffectManager>(
          qobject_cast<QWaylandObject *>(parent))
{
}

ExtBackgroundEffectManager::~ExtBackgroundEffectManager() = default;

void ExtBackgroundEffectManager::initialize()
{
    QWaylandCompositorExtensionTemplate<ExtBackgroundEffectManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    qInfo() << "Blur: ext_background_effect_v1 ready";
}

void ExtBackgroundEffectManager::ext_background_effect_manager_v1_get_background_effect(
    Resource *resource, uint32_t id, struct ::wl_resource *surface)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (!qs) {
        wl_resource_post_error(resource->handle, 0, "invalid surface");
        return;
    }
    QWaylandResource effectResource(wl_resource_create(
        resource->client(), &::ext_background_effect_surface_v1_interface,
        wl_resource_get_version(resource->handle), id));
    if (!effectResource.resource())
        return;
    auto *effect = new ExtBackgroundEffectSurface(qs, effectResource);
    m_effects.append(effect);
    connect(effect, &QObject::destroyed, this,
            [this, effect] { m_effects.removeOne(effect); });
}

void ExtBackgroundEffectManager::ext_background_effect_manager_v1_destroy(Resource *)
{
}
