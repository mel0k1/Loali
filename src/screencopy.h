#pragma once

#include <QImage>
#include <QObject>
#include <QPointer>
#include <QRect>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandQuickExtension>

#include "qwayland-server-wlr-screencopy-unstable-v1.h"

class QQuickWindow;

class ScreencopyFrame : public QObject, public QtWaylandServer::zwlr_screencopy_frame_v1
{
    Q_OBJECT
public:
    ScreencopyFrame(wl_client *client, uint32_t id, uint32_t version, QWaylandOutput *output,
                    const QRect &region, bool withDamage);

    void announce();

protected:
    void zwlr_screencopy_frame_v1_copy(Resource *resource, ::wl_resource *buffer) override;
    void zwlr_screencopy_frame_v1_copy_with_damage(Resource *resource, ::wl_resource *buffer) override;
    void zwlr_screencopy_frame_v1_destroy_resource(Resource *resource) override;

private:
    void copyBuffer(Resource *resource, ::wl_resource *buffer, bool withDamage);
    QImage grab() const;
    QRect pixelRect() const;

    QPointer<QWaylandOutput> m_output;
    QQuickWindow *m_window = nullptr;
    QRect m_region;
    bool m_withDamage = false;
    bool m_copied = false;
};

class ScreencopyManager : public QWaylandCompositorExtensionTemplate<ScreencopyManager>,
                          public QtWaylandServer::zwlr_screencopy_manager_v1
{
    Q_OBJECT
public:
    explicit ScreencopyManager(QObject *parent = nullptr);

    void initialize() override;

    using QtWaylandServer::zwlr_screencopy_manager_v1::interface;
    using QtWaylandServer::zwlr_screencopy_manager_v1::interfaceName;

protected:
    void zwlr_screencopy_manager_v1_capture_output(Resource *resource, uint32_t id, int32_t overlay_cursor,
                                                   ::wl_resource *output) override;
    void zwlr_screencopy_manager_v1_capture_output_region(Resource *resource, uint32_t id, int32_t overlay_cursor,
                                                          ::wl_resource *output, int32_t x, int32_t y,
                                                          int32_t width, int32_t height) override;

private:
    void capture(Resource *resource, uint32_t id, ::wl_resource *output, const QRect &region, bool withDamage);
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(ScreencopyManager)
