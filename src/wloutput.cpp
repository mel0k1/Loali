#include "wloutput.h"

#include <QWaylandCompositor>
#include <QWaylandOutput>
#include <QWindow>
#include <algorithm>
#include <cstring>
#include <wayland-server-protocol.h>

void WlOutputFix::outputBind(wl_client *client, void *userData, uint32_t version, uint32_t id)
{
    auto *self = static_cast<WlOutputFix *>(userData);
    wl_resource *resource = wl_resource_create(client, &wl_output_interface,
                                               std::min(version, 4u), id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    static const struct wl_output_interface impl = {
        [](wl_client *c, wl_resource *r) {
            Q_UNUSED(c)
            wl_resource_destroy(r);
        }
    };
    wl_resource_set_implementation(resource, &impl, self, nullptr);

    QWaylandOutput *out = self->m_compositor ? self->m_compositor->defaultOutput() : nullptr;
    const QPoint pos = out ? out->position() : QPoint(0, 0);
    const QSize phys = out && out->physicalSize().isValid() ? out->physicalSize() : QSize(320, 240);
    QWaylandOutputMode mode;
    if (out)
        mode = out->currentMode();
    if (!mode.isValid()) {
        if (out && out->window())
            mode = QWaylandOutputMode(out->window()->size(), 60000);
        else
            mode = QWaylandOutputMode(QSize(1280, 800), 60000);
    }

    wl_output_send_geometry(resource, pos.x(), pos.y(), phys.width(), phys.height(),
                            WL_OUTPUT_SUBPIXEL_UNKNOWN,
                            "Loali", "Loali Virtual Output",
                            WL_OUTPUT_TRANSFORM_NORMAL);
    wl_output_send_mode(resource, WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED,
                        mode.size().width(), mode.size().height(), mode.refreshRate());
    wl_output_send_scale(resource, 1);
    const uint32_t bound = wl_resource_get_version(resource);
    if (bound >= 4)
        wl_output_send_name(resource, "Loali-1");
    if (bound >= 4)
        wl_output_send_description(resource, "Loali compositor output");
    wl_output_send_done(resource);
}

bool WlOutputFix::globalFilter(const wl_client *client, const wl_global *global, void *data)
{
    auto *self = static_cast<WlOutputFix *>(data);
    const struct wl_interface *iface = wl_global_get_interface(global);
    const bool isOutput = iface && std::strcmp(iface->name, "wl_output") == 0;
    const bool foreign = isOutput && global != self->m_global;
    if (isOutput)
        qInfo() << "WlOutputFix: filter sees wl_output global" << global << "ours" << self->m_global
                << "foreign" << foreign << "for client" << client;
    return !foreign;
}

WlOutputFix::WlOutputFix(QObject *parent)
    : QObject(parent)
{
    m_retry.setInterval(400);
    connect(&m_retry, &QTimer::timeout, this, [this] {
        if (tryApply())
            m_retry.stop();
    });
    m_watch.setInterval(2000);
    connect(&m_watch, &QTimer::timeout, this, [this] {
        if (m_applied && !m_compositor.isNull() && m_global) {
            wl_display *display = m_compositor->display();
            if (display)
                wl_display_set_global_filter(display, &WlOutputFix::globalFilter, this);
        }
    });
    m_watch.start();
}

WlOutputFix::~WlOutputFix()
{
    m_retry.stop();
    m_watch.stop();
    if (m_global) {
        wl_global_remove(m_global);
        wl_global_destroy(m_global);
        m_global = nullptr;
    }
}

QVariant WlOutputFix::compositor() const
{
    return QVariant::fromValue(static_cast<QObject *>(m_compositor.data()));
}

void WlOutputFix::setCompositor(const QVariant &compositor)
{
    QWaylandCompositor *c = qvariant_cast<QWaylandCompositor *>(compositor);
    if (c == m_compositor)
        return;
    m_compositor = c;
    emit compositorChanged();
}

bool WlOutputFix::applied() const
{
    return m_applied;
}

void WlOutputFix::apply()
{
    if (tryApply())
        return;
    m_retry.start();
}

bool WlOutputFix::tryApply()
{
    if (m_applied)
        return true;
    if (m_compositor.isNull()) {
        qWarning() << "WlOutputFix: no compositor";
        return false;
    }
    wl_display *display = m_compositor->display();
    if (!display) {
        qWarning() << "WlOutputFix: no display yet";
        return false;
    }
    QWaylandOutput *out = m_compositor->defaultOutput();
    if (!out) {
        qWarning() << "WlOutputFix: no output yet";
        return false;
    }

    publish();
    if (!m_global) {
        qWarning() << "WlOutputFix: wl_global_create failed";
        return false;
    }
    wl_display_set_global_filter(display, &WlOutputFix::globalFilter, this);
    qInfo() << "WlOutputFix: own wl_output v4 global published, Qt global filtered";
    m_applied = true;
    emit appliedChanged();
    return true;
}

void WlOutputFix::publish()
{
    if (m_global || m_compositor.isNull())
        return;
    wl_display *display = m_compositor->display();
    if (!display)
        return;
    m_global = wl_global_create(display, &wl_output_interface, 4, this, &WlOutputFix::outputBind);
}
