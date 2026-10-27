#pragma once

#include <QObject>
#include <QRect>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandQuickExtension>

#include "qwayland-server-xdg-output-unstable-v1.h"

class XdgOutput : public QObject, public QtWaylandServer::zxdg_output_v1
{
    Q_OBJECT
public:
    XdgOutput(wl_client *client, uint32_t id, uint32_t version, QWaylandOutput *output);

protected:
    void zxdg_output_v1_destroy_resource(Resource *resource) override;

private:
    QWaylandOutput *m_output = nullptr;
};

class XdgOutputManager : public QWaylandCompositorExtensionTemplate<XdgOutputManager>,
                         public QtWaylandServer::zxdg_output_manager_v1
{
    Q_OBJECT
public:
    explicit XdgOutputManager(QObject *parent = nullptr);

    void initialize() override;

    using QtWaylandServer::zxdg_output_manager_v1::interface;
    using QtWaylandServer::zxdg_output_manager_v1::interfaceName;

protected:
    void zxdg_output_manager_v1_get_xdg_output(Resource *resource, uint32_t id, ::wl_resource *output) override;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(XdgOutputManager)
