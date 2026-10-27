#include "xdgoutput.h"

#include <QtWaylandCompositor/QWaylandCompositor>

XdgOutput::XdgOutput(wl_client *client, uint32_t id, uint32_t version, QWaylandOutput *output)
    : m_output(output)
{
    init(client, id, version);
    const QRect geo = m_output ? m_output->geometry() : QRect();
    send_logical_position(geo.x(), geo.y());
    send_logical_size(geo.width(), geo.height());
    if (version >= 2) {
        const QString name = m_output && !m_output->model().isEmpty() ? m_output->model() : QStringLiteral("WL-1");
        send_name(name);
        send_description(QStringLiteral("Loali compositor output"));
    }
    send_done();
}

void XdgOutput::zxdg_output_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    delete this;
}

XdgOutputManager::XdgOutputManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<XdgOutputManager>(parent)
{
}

void XdgOutputManager::initialize()
{
    QWaylandCompositorExtensionTemplate<XdgOutputManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 3);
}

void XdgOutputManager::zxdg_output_manager_v1_get_xdg_output(Resource *resource, uint32_t id, ::wl_resource *output)
{
    QWaylandOutput *out = QWaylandOutput::fromResource(output);
    if (!out)
        return;
    new XdgOutput(resource->client(), id, uint32_t(wl_resource_get_version(resource->handle)), out);
}
