#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <qqml.h>

#include <QtWaylandCompositor/QWaylandCompositorExtensionTemplate>
#include <QtWaylandCompositor/QWaylandQuickExtension>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandSeat>

#include "qwayland-server-ext-idle-notify-v1.h"
#include "qwayland-server-wlr-output-power-management-unstable-v1.h"

class Idle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool blanked READ blanked NOTIFY blankedChanged)

public:
    static Idle *instance();
    static void activity();
    static void suppress();
    static void release();
    static bool suppressed();
    static quint64 idleMs();

    bool blanked() const { return m_blanked; }
    void setBlanked(bool value);
    void wake();

    static void registerTypes();
    static QObject *create(QQmlEngine *e, QJSEngine *j);

signals:
    void activityOccurred();
    void blankedChanged();

private:
    explicit Idle(QObject *parent = nullptr);

    static Idle *s_instance;
    bool m_blanked = false;
    int m_suppression = 0;
    QElapsedTimer m_idleTimer;
    QTimer *m_autoBlank = nullptr;
};

class IdleNotification : public QObject, public QtWaylandServer::ext_idle_notification_v1
{
    Q_OBJECT
public:
    IdleNotification(struct ::wl_client *client, uint32_t id, uint32_t version, uint32_t timeout);
    ~IdleNotification() override;

    using QtWaylandServer::ext_idle_notification_v1::interface;
    using QtWaylandServer::ext_idle_notification_v1::interfaceName;

    void onActivity();

protected:
    void ext_idle_notification_v1_destroy(Resource *resource) override;
    void ext_idle_notification_v1_destroy_resource(Resource *resource) override;

private:
    void armTimer();

    QTimer *m_timer = nullptr;
    bool m_idled = false;
};

class IdleManager : public QWaylandCompositorExtensionTemplate<IdleManager>,
                    public QtWaylandServer::ext_idle_notifier_v1
{
    Q_OBJECT
public:
    explicit IdleManager(QObject *parent = nullptr);
    ~IdleManager() override;
    void initialize() override;

    using QtWaylandServer::ext_idle_notifier_v1::interface;
    using QtWaylandServer::ext_idle_notifier_v1::interfaceName;

    static IdleManager *instance() { return s_instance; }

    void notifyActivity();

protected:
    void ext_idle_notifier_v1_get_idle_notification(Resource *resource, uint32_t id, uint32_t timeout,
                                                    ::wl_resource *seat) override;

private:
    static IdleManager *s_instance;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(IdleManager)

class OutputPower : public QObject, public QtWaylandServer::zwlr_output_power_v1
{
    Q_OBJECT
public:
    OutputPower(struct ::wl_client *client, uint32_t id, uint32_t version);
    ~OutputPower() override;

    using QtWaylandServer::zwlr_output_power_v1::interface;
    using QtWaylandServer::zwlr_output_power_v1::interfaceName;

    void pushMode(bool blanked);

protected:
    void zwlr_output_power_v1_set_mode(Resource *resource, uint32_t mode) override;
    void zwlr_output_power_v1_destroy(Resource *resource) override;
    void zwlr_output_power_v1_destroy_resource(Resource *resource) override;
};

class OutputPowerManager : public QWaylandCompositorExtensionTemplate<OutputPowerManager>,
                           public QtWaylandServer::zwlr_output_power_manager_v1
{
    Q_OBJECT
public:
    explicit OutputPowerManager(QObject *parent = nullptr);
    ~OutputPowerManager() override;
    void initialize() override;

    using QtWaylandServer::zwlr_output_power_manager_v1::interface;
    using QtWaylandServer::zwlr_output_power_manager_v1::interfaceName;

    static QList<OutputPower *> &devices() { return s_devices; }

protected:
    void zwlr_output_power_manager_v1_get_output_power(Resource *resource, uint32_t id,
                                                      ::wl_resource *output) override;

private:
    static QList<OutputPower *> s_devices;
};

Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(OutputPowerManager)
