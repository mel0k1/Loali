#include "pwcast.h"

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

#include <spa/utils/result.h>

namespace {

QMutex g_frameMutex;
QImage g_latestFrame;

}

void PwScreenCast::onProcess(void *data)
{
    auto *self = static_cast<PwScreenCast *>(data);
    struct pw_buffer *buf = pw_stream_dequeue_buffer(self->m_stream);
    if (!buf)
        return;
    struct spa_buffer *spaBuf = buf->buffer;
    QImage frame;
    {
        QMutexLocker lock(&g_frameMutex);
        frame = g_latestFrame;
    }
    if (spaBuf && spaBuf->datas[0].data && !frame.isNull()) {
        const int stride = int(spaBuf->datas[0].maxsize) / frame.height();
        QImage converted = frame.convertToFormat(QImage::Format_RGBX8888);
        const int bytes = qMin(converted.bytesPerLine(), stride);
        for (int y = 0; y < frame.height(); ++y)
            memcpy(spaBuf->datas[0].data + y * stride,
                   converted.constBits() + y * converted.bytesPerLine(), bytes);
        spaBuf->datas[0].chunk->offset = 0;
        spaBuf->datas[0].chunk->stride = stride;
        spaBuf->datas[0].chunk->size = stride * frame.height();
    }
    pw_stream_queue_buffer(self->m_stream, buf);
}

void PwScreenCast::onParamChanged(void *data, uint32_t id, const struct spa_pod *param)
{
    auto *self = static_cast<PwScreenCast *>(data);
    if (param == nullptr || id != SPA_PARAM_Format)
        return;
    spa_format_video_raw_parse(param, &self->m_format);
    self->m_negotiated = true;
    if (self->m_nodeId == 0) {
        self->m_nodeId = pw_stream_get_node_id(self->m_stream);
        emit self->nodeReady(self->m_nodeId);
    }
}

void PwScreenCast::onStateChanged(void *data, pw_stream_state, pw_stream_state state,
                                  const char *error)
{
    auto *self = static_cast<PwScreenCast *>(data);
    if (state == PW_STREAM_STATE_ERROR) {
        qWarning() << "PwScreenCast error:" << (error ? error : "unknown");
        return;
    }
    if (state >= PW_STREAM_STATE_CONNECTING && self->m_nodeId == 0) {
        self->m_nodeId = pw_stream_get_node_id(self->m_stream);
        if (self->m_nodeId != 0)
            emit self->nodeReady(self->m_nodeId);
    }
}

namespace {

const struct pw_stream_events streamEvents = [] {
    struct pw_stream_events ev {};
    memset(&ev, 0, sizeof(ev));
    ev.version = PW_VERSION_STREAM_EVENTS;
    ev.process = PwScreenCast::onProcess;
    ev.param_changed = PwScreenCast::onParamChanged;
    ev.state_changed = PwScreenCast::onStateChanged;
    return ev;
}();

}

PwScreenCast *PwScreenCast::instance()
{
    static PwScreenCast inst;
    return &inst;
}

PwScreenCast::PwScreenCast(QObject *parent)
    : QObject(parent)
{
}

PwScreenCast::~PwScreenCast()
{
    stop();
}

bool PwScreenCast::start()
{
    if (m_loop)
        return true;
    pw_init(nullptr, nullptr);
    m_loop = pw_thread_loop_new("loali-pwcast", nullptr);
    if (!m_loop)
        return false;
    m_stream = pw_stream_new_simple(pw_thread_loop_get_loop(m_loop), "loali-screen-cast",
                                    pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                                                      PW_KEY_MEDIA_CATEGORY, "Capture",
                                                      PW_KEY_MEDIA_ROLE, "Screen",
                                                      PW_KEY_STREAM_IS_LIVE, "true",
                                                      PW_KEY_NODE_NAME, "loali_screen_cast",
                                                      nullptr),
                                    &streamEvents, this);
    if (!m_stream) {
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
        return false;
    }
    const int width = 1280;
    const int height = 800;
    uint8_t buffer[2048];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_rectangle defRect = SPA_RECTANGLE(width, height);
    const struct spa_rectangle minRect = SPA_RECTANGLE(64, 64);
    const struct spa_rectangle maxRect = SPA_RECTANGLE(8192, 8192);
    const struct spa_fraction defFps = SPA_FRACTION(30, 1);
    const struct spa_fraction minFps = SPA_FRACTION(1, 1);
    const struct spa_fraction maxFps = SPA_FRACTION(60, 1);
    const struct spa_pod *params[1];
    params[0] = static_cast<const struct spa_pod *>(spa_pod_builder_add_object(
        &b, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format,
        SPA_POD_CHOICE_ENUM_Id(4, SPA_VIDEO_FORMAT_BGRx, SPA_VIDEO_FORMAT_BGRx,
                               SPA_VIDEO_FORMAT_RGBx, SPA_VIDEO_FORMAT_RGBA),
        SPA_FORMAT_VIDEO_size,
        SPA_POD_CHOICE_RANGE_Rectangle(&defRect, &minRect, &maxRect),
        SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&defFps),
        SPA_FORMAT_VIDEO_maxFramerate,
        SPA_POD_CHOICE_RANGE_Fraction(&defFps, &minFps, &maxFps)));
    if (pw_stream_connect(m_stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
                          static_cast<pw_stream_flags>(PW_STREAM_FLAG_DRIVER
                                                       | PW_STREAM_FLAG_ALLOC_BUFFERS),
                          params, 1) < 0) {
        pw_stream_destroy(m_stream);
        m_stream = nullptr;
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
        return false;
    }
    pw_thread_loop_start(m_loop);
    return true;
}

void PwScreenCast::stop()
{
    if (m_loop) {
        pw_thread_loop_stop(m_loop);
        if (m_stream) {
            pw_stream_destroy(m_stream);
            m_stream = nullptr;
        }
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
    }
    m_nodeId = 0;
    m_negotiated = false;
}

void PwScreenCast::setFrame(const QImage &frame)
{
    QMutexLocker lock(&g_frameMutex);
    g_latestFrame = frame;
}
