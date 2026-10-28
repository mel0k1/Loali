#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QVariant>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandSurface>

#include "qwayland-server-ext-background-effect-v1.h"
#include "qwayland-server-blur.h"

class QWaylandCompositor;
class KdeBlur;
class ExtBackgroundEffectSurface;

class BlurRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
public:
    static BlurRegistry *instance()
    {
        if (!s_instance)
            s_instance = new BlurRegistry();
        return s_instance;
    }

    static QObject *create(QQmlEngine *qml, QJSEngine *js)
    {
        Q_UNUSED(js)
        auto *inst = instance();
        if (qml)
            qml->setObjectOwnership(inst, QQmlEngine::CppOwnership);
        return inst;
    }

    int revision() const { return m_revision; }

    void setBlurred(QWaylandSurface *surface, bool on)
    {
        if (!surface)
            return;
        const bool had = m_blurred.contains(surface);
        if (on) {
            if (had)
                return;
            connect(surface, &QObject::destroyed, this, [this, surface] {
                m_blurred.remove(surface);
                bump();
            });
            m_blurred.insert(surface, true);
        } else if (had) {
            m_blurred.remove(surface);
        } else {
            return;
        }
        bump();
    }

    Q_INVOKABLE bool blurred(const QVariant &surface) const
    {
        auto *s = qvariant_cast<QWaylandSurface *>(surface);
        return s && m_blurred.contains(s);
    }

signals:
    void revisionChanged();

private:
    explicit BlurRegistry(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void bump()
    {
        ++m_revision;
        emit revisionChanged();
    }

    static BlurRegistry *s_instance;
    QHash<QWaylandSurface *, bool> m_blurred;
    int m_revision = 0;
};

class KdeBlur : public QObject, public QtWaylandServer::org_kde_kwin_blur
{
    Q_OBJECT
public:
    KdeBlur(QWaylandSurface *surface, const QWaylandResource &resource);
    ~KdeBlur() override;

    using QtWaylandServer::org_kde_kwin_blur::interface;
    using QtWaylandServer::org_kde_kwin_blur::interfaceName;

protected:
    void org_kde_kwin_blur_set_region(Resource *resource, struct ::wl_resource *region) override;
    void org_kde_kwin_blur_commit(Resource *resource) override;
    void org_kde_kwin_blur_release(Resource *resource) override;
    void org_kde_kwin_blur_destroy_resource(Resource *resource) override;

private:
    QPointer<QWaylandSurface> m_surface;

    friend class KdeBlurManager;
};

class KdeBlurManager : public QWaylandCompositorExtensionTemplate<KdeBlurManager>,
                       public QtWaylandServer::org_kde_kwin_blur_manager
{
    Q_OBJECT
public:
    explicit KdeBlurManager(QObject *parent = nullptr);
    ~KdeBlurManager() override;
    void initialize() override;

    using QtWaylandServer::org_kde_kwin_blur_manager::interface;
    using QtWaylandServer::org_kde_kwin_blur_manager::interfaceName;

protected:
    void org_kde_kwin_blur_manager_create(Resource *resource, uint32_t id,
                                          struct ::wl_resource *surface) override;
    void org_kde_kwin_blur_manager_unset(Resource *resource,
                                         struct ::wl_resource *surface) override;
    void org_kde_kwin_blur_manager_destroy_resource(Resource *resource) override;

private:
    QList<KdeBlur *> m_blurs;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(KdeBlurManager)

class ExtBackgroundEffectSurface : public QObject,
                                   public QtWaylandServer::ext_background_effect_surface_v1
{
    Q_OBJECT
public:
    ExtBackgroundEffectSurface(QWaylandSurface *surface, const QWaylandResource &resource);
    ~ExtBackgroundEffectSurface() override;

    using QtWaylandServer::ext_background_effect_surface_v1::interface;
    using QtWaylandServer::ext_background_effect_surface_v1::interfaceName;

protected:
    void ext_background_effect_surface_v1_set_blur_region(Resource *resource,
                                                          struct ::wl_resource *region) override;
    void ext_background_effect_surface_v1_destroy(Resource *resource) override;
    void ext_background_effect_surface_v1_destroy_resource(Resource *resource) override;

private:
    QPointer<QWaylandSurface> m_surface;
};

class ExtBackgroundEffectManager :
        public QWaylandCompositorExtensionTemplate<ExtBackgroundEffectManager>,
        public QtWaylandServer::ext_background_effect_manager_v1
{
    Q_OBJECT
public:
    explicit ExtBackgroundEffectManager(QObject *parent = nullptr);
    ~ExtBackgroundEffectManager() override;
    void initialize() override;

    using QtWaylandServer::ext_background_effect_manager_v1::interface;
    using QtWaylandServer::ext_background_effect_manager_v1::interfaceName;

protected:
    void ext_background_effect_manager_v1_get_background_effect(Resource *resource, uint32_t id,
                                                                struct ::wl_resource *surface) override;
    void ext_background_effect_manager_v1_destroy(Resource *resource) override;

private:
    QList<ExtBackgroundEffectSurface *> m_effects;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(ExtBackgroundEffectManager)
