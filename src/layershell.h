#pragma once

#include <QMargins>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandQuickShellIntegration>
#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandShellSurface>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandSurfaceRole>

#include "qwayland-server-wlr-layer-shell-unstable-v1.h"

class QWaylandCompositor;
class QWaylandOutput;
class LayerShell;
class LayerSurface;

class LayerShellIntegration : public QWaylandQuickShellIntegration
{
public:
    explicit LayerShellIntegration(QWaylandQuickShellSurfaceItem *item);

private:
    QWaylandQuickShellSurfaceItem *m_item = nullptr;
};

class LayerSurface : public QWaylandShellSurfaceTemplate<LayerSurface>,
                     public QtWaylandServer::zwlr_layer_surface_v1
{
    Q_OBJECT
    Q_PROPERTY(uint layer READ layer NOTIFY layerChanged)
    Q_PROPERTY(uint anchor READ anchor NOTIFY stateChanged)
    Q_PROPERTY(int exclusiveZone READ exclusiveZone NOTIFY stateChanged)
    Q_PROPERTY(uint keyboardInteractivity READ keyboardInteractivity NOTIFY stateChanged)
    Q_PROPERTY(QString namespaceName READ namespaceName CONSTANT)
    Q_PROPERTY(QRect placement READ placement NOTIFY placementChanged)
    Q_PROPERTY(int edgeInsetLeft READ edgeInsetLeft NOTIFY stateChanged)
    Q_PROPERTY(int edgeInsetTop READ edgeInsetTop NOTIFY stateChanged)
    Q_PROPERTY(int edgeInsetRight READ edgeInsetRight NOTIFY stateChanged)
    Q_PROPERTY(int edgeInsetBottom READ edgeInsetBottom NOTIFY stateChanged)

public:
    LayerSurface(LayerShell *shell, QWaylandSurface *surface, ::wl_resource *output,
                 uint32_t layer, const QString &ns, const QWaylandResource &resource);
    ~LayerSurface() override;

    using QtWaylandServer::zwlr_layer_surface_v1::interface;
    using QtWaylandServer::zwlr_layer_surface_v1::interfaceName;

    static QWaylandSurfaceRole *role();
    static LayerSurface *fromResource(::wl_resource *resource);

    QWaylandQuickShellIntegration *createIntegration(QWaylandQuickShellSurfaceItem *item) override;

    QWaylandSurface *surface() const { return m_surface.data(); }
    uint layer() const { return m_layer; }
    uint anchor() const { return m_anchor; }
    int exclusiveZone() const { return m_exclusiveZone; }
    uint keyboardInteractivity() const { return m_keyboard; }
    QString namespaceName() const { return m_namespace; }
    QRect placement() const { return m_placement; }

    int edgeInsetLeft() const { return m_insetLeft; }
    int edgeInsetTop() const { return m_insetTop; }
    int edgeInsetRight() const { return m_insetRight; }
    int edgeInsetBottom() const { return m_insetBottom; }

    void refresh();

signals:
    void layerChanged();
    void stateChanged();
    void placementChanged();
    void surfaceClosed(LayerSurface *surface);

protected:
    void zwlr_layer_surface_v1_set_size(Resource *resource, uint32_t width, uint32_t height) override;
    void zwlr_layer_surface_v1_set_anchor(Resource *resource, uint32_t anchor) override;
    void zwlr_layer_surface_v1_set_exclusive_zone(Resource *resource, int32_t zone) override;
    void zwlr_layer_surface_v1_set_margin(Resource *resource, int32_t top, int32_t right,
                                          int32_t bottom, int32_t left) override;
    void zwlr_layer_surface_v1_set_keyboard_interactivity(Resource *resource,
                                                          uint32_t interactivity) override;
    void zwlr_layer_surface_v1_set_layer(Resource *resource, uint32_t layer) override;
    void zwlr_layer_surface_v1_get_popup(Resource *resource, ::wl_resource *popup,
                                         ::wl_resource *parent);
    void zwlr_layer_surface_v1_ack_configure(Resource *resource, uint32_t serial) override;
    void zwlr_layer_surface_v1_destroy(Resource *resource) override;
    void zwlr_layer_surface_v1_destroy_resource(Resource *resource) override;

private:
    QWaylandOutput *output() const;
    QSize computeSize() const;
    QRect computePlacement(const QSize &size) const;
    void updateInsets();
    void maybeConfigure();
    void grabKeyboard();

    static QWaylandSurfaceRole s_role_impl;

    QPointer<QWaylandCompositor> m_compositor;
    QPointer<QWaylandSurface> m_surface;
    QPointer<QWaylandOutput> m_output;
    uint32_t m_layer = 2;
    uint32_t m_anchor = 0;
    int32_t m_exclusiveZone = 0;
    int32_t m_marginTop = 0;
    int32_t m_marginRight = 0;
    int32_t m_marginBottom = 0;
    int32_t m_marginLeft = 0;
    uint32_t m_keyboard = 0;
    QString m_namespace;
    QSize m_requestedSize;
    QRect m_placement;
    int m_insetLeft = 0;
    int m_insetTop = 0;
    int m_insetRight = 0;
    int m_insetBottom = 0;
    uint32_t m_pendingSerial = 0;
    bool m_initialConfigured = false;
};

class LayerShell : public QWaylandCompositorExtensionTemplate<LayerShell>,
                   public QtWaylandServer::zwlr_layer_shell_v1
{
    Q_OBJECT
public:
    explicit LayerShell(QObject *parent = nullptr);
    void initialize() override;

    using QtWaylandServer::zwlr_layer_shell_v1::interface;
    using QtWaylandServer::zwlr_layer_shell_v1::interfaceName;

signals:
    void layerSurfaceCreated(LayerSurface *surface);

protected:
    void zwlr_layer_shell_v1_get_layer_surface(Resource *resource, uint32_t id,
                                               ::wl_resource *surface, ::wl_resource *output,
                                               uint32_t layer, const QString &namespace_name) override;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(LayerShell)
