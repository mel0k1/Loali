#include "screencopy.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QtWaylandCompositor/QWaylandCompositor>

#include <cstring>
#include <ctime>

ScreencopyFrame::ScreencopyFrame(wl_client *client, uint32_t id, uint32_t version, QWaylandOutput *output,
                                 const QRect &region, bool withDamage)
    : m_output(output)
    , m_region(region)
    , m_version(version)
    , m_withDamage(withDamage)
{
    const QList<QWindow *> windows = QGuiApplication::topLevelWindows();
    for (QWindow *w : windows) {
        m_window = qobject_cast<QQuickWindow *>(w);
        if (m_window)
            break;
    }
    init(client, id, version);
    announce();
}

QRect ScreencopyFrame::pixelRect() const
{
    if (!m_output)
        return QRect();
    const QRect geo = m_output->geometry();
    const qreal dpr = m_window ? m_window->devicePixelRatio() : 1.0;
    QRect rect(qRound(geo.x() * dpr), qRound(geo.y() * dpr), qRound(geo.width() * dpr), qRound(geo.height() * dpr));
    if (!m_region.isNull()) {
        const QRect region(qRound(m_region.x() * dpr), qRound(m_region.y() * dpr),
                           qRound(m_region.width() * dpr), qRound(m_region.height() * dpr));
        rect = rect.intersected(region);
    }
    return rect;
}

void ScreencopyFrame::announce()
{
    const QRect rect = pixelRect();
    if (rect.isEmpty()) {
        send_failed();
        return;
    }
    send_buffer(0, uint32_t(rect.width()), uint32_t(rect.height()), uint32_t(rect.width() * 4));
    if (m_version >= 3)
        send_buffer_done();
}

QImage ScreencopyFrame::grab() const
{
    if (!m_window)
        return QImage();
    QImage img = m_window->grabWindow();
    if (img.isNull())
        return QImage();
    img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QRect rect = img.rect().intersected(pixelRect());
    if (rect.isEmpty())
        return QImage();
    return img.copy(rect);
}

void ScreencopyFrame::zwlr_screencopy_frame_v1_copy(Resource *resource, ::wl_resource *buffer)
{
    copyBuffer(resource, buffer, false);
}

void ScreencopyFrame::zwlr_screencopy_frame_v1_copy_with_damage(Resource *resource, ::wl_resource *buffer)
{
    copyBuffer(resource, buffer, true);
}

void ScreencopyFrame::copyBuffer(Resource *resource, ::wl_resource *buffer, bool withDamage)
{
    if (m_copied) {
        wl_resource_post_error(resource->handle, 0, "frame already used");
        return;
    }
    m_copied = true;
    wl_shm_buffer *shm = wl_shm_buffer_get(buffer);
    if (!shm) {
        wl_resource_post_error(resource->handle, 1, "unsupported buffer type");
        return;
    }
    const QImage img = grab();
    if (img.isNull()) {
        send_failed();
        return;
    }
    const int bw = wl_shm_buffer_get_width(shm);
    const int bh = wl_shm_buffer_get_height(shm);
    const int stride = wl_shm_buffer_get_stride(shm);
    if (bw != img.width() || bh != img.height() || stride < bw * 4) {
        send_failed();
        return;
    }
    wl_shm_buffer_begin_access(shm);
    uchar *dst = static_cast<uchar *>(wl_shm_buffer_get_data(shm));
    for (int y = 0; y < bh; ++y)
        std::memcpy(dst + y * stride, img.constScanLine(y), size_t(bw) * 4);
    wl_shm_buffer_end_access(shm);
    if (withDamage)
        send_damage(0, 0, uint32_t(bw), uint32_t(bh));
    send_flags(0);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const uint64_t secs = uint64_t(ts.tv_sec);
    send_ready(uint32_t(secs >> 32), uint32_t(secs & 0xffffffffULL), uint32_t(ts.tv_nsec));
}

void ScreencopyFrame::zwlr_screencopy_frame_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    delete this;
}

ScreencopyManager::ScreencopyManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<ScreencopyManager>()
{
    Q_UNUSED(parent)
}

void ScreencopyManager::initialize()
{
    QWaylandCompositorExtensionTemplate<ScreencopyManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 3);
}

void ScreencopyManager::capture(Resource *resource, uint32_t id, ::wl_resource *output, const QRect &region,
                                bool withDamage)
{
    QWaylandOutput *out = QWaylandOutput::fromResource(output);
    if (!out) {
        auto *comp = qobject_cast<QWaylandCompositor *>(extensionContainer());
        if (comp)
            out = comp->defaultOutput();
    }
    if (!out)
        return;
    new ScreencopyFrame(resource->client(), id, uint32_t(wl_resource_get_version(resource->handle)), out, region,
                        withDamage);
}

void ScreencopyManager::zwlr_screencopy_manager_v1_capture_output(Resource *resource, uint32_t id,
                                                                  int32_t overlay_cursor, ::wl_resource *output)
{
    Q_UNUSED(overlay_cursor)
    capture(resource, id, output, QRect(), false);
}

void ScreencopyManager::zwlr_screencopy_manager_v1_capture_output_region(Resource *resource, uint32_t id,
                                                                         int32_t overlay_cursor, ::wl_resource *output,
                                                                         int32_t x, int32_t y, int32_t width,
                                                                         int32_t height)
{
    Q_UNUSED(overlay_cursor)
    capture(resource, id, output, QRect(x, y, width, height), false);
}
