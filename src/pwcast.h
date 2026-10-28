#pragma once

#include <QObject>
#include <QImage>

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

class PwScreenCast : public QObject
{
    Q_OBJECT

public:
    static PwScreenCast *instance();
    bool start();
    void stop();
    bool active() const { return m_stream != nullptr; }
    uint32_t nodeId() const { return m_nodeId; }
    void setFrame(const QImage &frame);

    static void onProcess(void *data);
    static void onParamChanged(void *data, uint32_t id, const struct spa_pod *param);
    static void onStateChanged(void *data, pw_stream_state old, pw_stream_state state,
                               const char *error);

signals:
    void nodeReady(uint32_t nodeId);

private:
    explicit PwScreenCast(QObject *parent = nullptr);
    ~PwScreenCast() override;

    struct pw_thread_loop *m_loop = nullptr;
    struct pw_stream *m_stream = nullptr;
    struct spa_video_info_raw m_format {};
    uint32_t m_nodeId = 0;
    bool m_negotiated = false;
};
