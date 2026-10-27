#include "layershell.h"

#include <QMetaObject>

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandSeat>

LayerShellIntegration::LayerShellIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration(item)
    , m_item(item)
{
    if (auto *surf = qobject_cast<LayerSurface *>(m_item->shellSurface()))
        m_item->setSurface(surf->surface());
}

LayerSurface::LayerSurface(LayerShell *shell, QWaylandSurface *surface, ::wl_resource *output,
                           uint32_t layer, const QString &ns, const QWaylandResource &resource)
    : m_compositor(shell ? qobject_cast<QWaylandCompositor *>(shell->extensionContainer()) : nullptr)
    , m_surface(surface)
    , m_layer(layer > 3 ? 2u : layer)
    , m_namespace(ns)
{
    init(resource.resource());
    setExtensionContainer(surface);
    QWaylandCompositorExtension::initialize();

    if (output)
        m_output = QWaylandOutput::fromResource(output);

    if (m_output) {
        connect(m_output, &QWaylandOutput::currentModeChanged, this, &LayerSurface::refresh);
        connect(m_output, &QWaylandOutput::positionChanged, this, &LayerSurface::refresh);
    }

    QMetaObject::invokeMethod(this, [this] { maybeConfigure(); }, Qt::QueuedConnection);
}

LayerSurface::~LayerSurface() = default;

QWaylandSurfaceRole LayerSurface::s_role_impl("zwlr_layer_surface_v1");

QWaylandSurfaceRole *LayerSurface::role()
{
    return &s_role_impl;
}

LayerSurface *LayerSurface::fromResource(::wl_resource *resource)
{
    auto *res = Resource::fromResource(resource);
    return res ? static_cast<LayerSurface *>(res->object()) : nullptr;
}

QWaylandQuickShellIntegration *LayerSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new LayerShellIntegration(item);
}

void LayerSurface::zwlr_layer_surface_v1_set_size(Resource *, uint32_t width, uint32_t height)
{
    m_requestedSize = QSize(int(width), int(height));
}

void LayerSurface::zwlr_layer_surface_v1_set_anchor(Resource *, uint32_t anchor)
{
    m_anchor = anchor;
}

void LayerSurface::zwlr_layer_surface_v1_set_exclusive_zone(Resource *, int32_t zone)
{
    m_exclusiveZone = zone;
}

void LayerSurface::zwlr_layer_surface_v1_set_margin(Resource *, int32_t top, int32_t right,
                                                    int32_t bottom, int32_t left)
{
    m_marginTop = top;
    m_marginRight = right;
    m_marginBottom = bottom;
    m_marginLeft = left;
}

void LayerSurface::zwlr_layer_surface_v1_set_keyboard_interactivity(Resource *, uint32_t ki)
{
    m_keyboard = ki > 2 ? 0u : ki;
}

void LayerSurface::zwlr_layer_surface_v1_set_layer(Resource *, uint32_t layer)
{
    if (layer > 3)
        return;
    m_layer = layer;
    emit layerChanged();
    emit stateChanged();
}

void LayerSurface::zwlr_layer_surface_v1_get_popup(Resource *, ::wl_resource *, ::wl_resource *)
{
}

void LayerSurface::zwlr_layer_surface_v1_ack_configure(Resource *, uint32_t)
{
    m_initialConfigured = true;
    grabKeyboard();
}

void LayerSurface::zwlr_layer_surface_v1_destroy(Resource *)
{
}

void LayerSurface::zwlr_layer_surface_v1_destroy_resource(Resource *)
{
    emit surfaceClosed(this);
    delete this;
}

void LayerSurface::refresh()
{
    maybeConfigure();
}

QWaylandOutput *LayerSurface::output() const
{
    if (m_output)
        return m_output;
    return m_compositor ? m_compositor->defaultOutput() : nullptr;
}

QSize LayerSurface::computeSize() const
{
    const QRect geo = output() ? output()->geometry() : QRect(0, 0, 1280, 800);
    int w = m_requestedSize.width();
    int h = m_requestedSize.height();
    const bool bothX = (m_anchor & 4u) && (m_anchor & 8u);
    const bool bothY = (m_anchor & 1u) && (m_anchor & 2u);
    if (w <= 0)
        w = bothX ? geo.width() - m_marginLeft - m_marginRight : geo.width();
    if (h <= 0)
        h = bothY ? geo.height() - m_marginTop - m_marginBottom : geo.height();
    w = qBound(1, w, geo.width());
    h = qBound(1, h, geo.height());
    return QSize(w, h);
}

QRect LayerSurface::computePlacement(const QSize &size) const
{
    const QRect geo = output() ? output()->geometry() : QRect(0, 0, 1280, 800);
    int x = 0;
    int y = 0;
    if (m_anchor & 4u)
        x = geo.x() + m_marginLeft;
    else if (m_anchor & 8u)
        x = geo.x() + geo.width() - size.width() - m_marginRight;
    else
        x = geo.x() + (geo.width() - size.width()) / 2;

    if (m_anchor & 1u)
        y = geo.y() + m_marginTop;
    else if (m_anchor & 2u)
        y = geo.y() + geo.height() - size.height() - m_marginBottom;
    else
        y = geo.y() + (geo.height() - size.height()) / 2;

    return QRect(x, y, size.width(), size.height());
}

void LayerSurface::updateInsets()
{
    m_insetLeft = 0;
    m_insetTop = 0;
    m_insetRight = 0;
    m_insetBottom = 0;
    if (m_exclusiveZone < 0)
        return;
    const int zone = m_exclusiveZone;
    const bool top = m_anchor & 1u;
    const bool bottom = m_anchor & 2u;
    const bool left = m_anchor & 4u;
    const bool right = m_anchor & 8u;
    if (top && !bottom)
        m_insetTop = zone + qMax(0, m_marginTop);
    if (bottom && !top)
        m_insetBottom = zone + qMax(0, m_marginBottom);
    if (left && !right)
        m_insetLeft = zone + qMax(0, m_marginLeft);
    if (right && !left)
        m_insetRight = zone + qMax(0, m_marginRight);
}

void LayerSurface::maybeConfigure()
{
    const QSize size = computeSize();
    m_placement = computePlacement(size);
    updateInsets();
    emit placementChanged();
    emit stateChanged();

    auto *comp = m_compositor.data();
    if (!comp)
        return;
    m_pendingSerial = comp->nextSerial();
    send_configure(m_pendingSerial, uint32_t(size.width()), uint32_t(size.height()));
}

void LayerSurface::grabKeyboard()
{
    if (m_keyboard != 1)
        return;
    auto *comp = m_compositor.data();
    if (!comp || !m_surface)
        return;
    if (QWaylandSeat *seat = comp->defaultSeat())
        seat->setKeyboardFocus(m_surface);
}

LayerShell::LayerShell(QObject *parent)
    : QWaylandCompositorExtensionTemplate<LayerShell>(parent)
{
}

void LayerShell::initialize()
{
    QWaylandCompositorExtensionTemplate<LayerShell>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 4);
    qInfo() << "LayerShell: zwlr_layer_shell_v1 ready";
}

void LayerShell::zwlr_layer_shell_v1_get_layer_surface(Resource *resource, uint32_t id,
                                                       ::wl_resource *surface, ::wl_resource *output,
                                                       uint32_t layer, const QString &namespace_)
{
    QWaylandSurface *qs = QWaylandSurface::fromResource(surface);
    if (!qs) {
        wl_resource_post_error(resource->handle, ZWLR_LAYER_SHELL_V1_ERROR_ROLE,
                               "invalid wl_surface");
        return;
    }
    if (layer > 3) {
        wl_resource_post_error(resource->handle, ZWLR_LAYER_SHELL_V1_ERROR_INVALID_LAYER,
                               "invalid layer %u", layer);
        return;
    }
    if (!qs->setRole(LayerSurface::role(), resource->handle, ZWLR_LAYER_SHELL_V1_ERROR_ROLE))
        return;

    QWaylandResource surfResource(wl_resource_create(resource->client(),
                                                     &::zwlr_layer_surface_v1_interface,
                                                     wl_resource_get_version(resource->handle), id));
    if (!surfResource.resource())
        return;

    LayerSurface *ls = new LayerSurface(this, qs, output, layer, namespace_, surfResource);
    emit layerSurfaceCreated(ls);
}

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(LayerShell)
