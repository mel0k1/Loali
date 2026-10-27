#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include <qqml.h>
#include <wayland-server-core.h>

class QWaylandCompositor;

class WlOutputFix : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant compositor READ compositor WRITE setCompositor NOTIFY compositorChanged)
    Q_PROPERTY(bool applied READ applied NOTIFY appliedChanged)

public:
    explicit WlOutputFix(QObject *parent = nullptr);
    ~WlOutputFix() override;

    QVariant compositor() const;
    void setCompositor(const QVariant &compositor);
    bool applied() const;

    Q_INVOKABLE void apply();

signals:
    void compositorChanged();
    void appliedChanged();

private:
    bool tryApply();
    void publish();

    static void outputBind(wl_client *client, void *userData, uint32_t version, uint32_t id);
    static bool globalFilter(const wl_client *client, const wl_global *global, void *data);

    QPointer<QWaylandCompositor> m_compositor;
    struct wl_global *m_global = nullptr;
    bool m_applied = false;
    QTimer m_retry;

    friend void outputBindThunk(wl_client *, void *, uint32_t, uint32_t);
};
