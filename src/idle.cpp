#include "idle.h"

#include <QGuiApplication>
#include <QQmlEngine>
#include <QTimer>
#include <QtWaylandCompositor/QWaylandCompositor>

Idle *Idle::s_instance = nullptr;

Idle::Idle(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    m_idleTimer.start();
    m_autoBlank = new QTimer(this);
    m_autoBlank->setSingleShot(true);
    m_autoBlank->setInterval(300000);
    connect(m_autoBlank, &QTimer::timeout, this, [this] {
        if (m_suppression > 0) {
            m_autoBlank->start();
            return;
        }
        if (!m_blanked) {
            m_blanked = true;
            emit blankedChanged();
            for (OutputPower *dev : OutputPowerManager::devices())
                dev->pushMode(true);
            qInfo() << "Idle: auto blank after inactivity";
        }
    });
    m_autoBlank->start();
    connect(this, &Idle::activityOccurred, this, [this] { m_autoBlank->start(); });
}

Idle *Idle::instance()
{
    if (!s_instance)
        new Idle(QGuiApplication::instance());
    return s_instance;
}

void Idle::activity()
{
    Idle *self = instance();
    self->m_idleTimer.restart();
    emit self->activityOccurred();
    if (self->m_blanked) {
        self->m_blanked = false;
        emit self->blankedChanged();
        for (OutputPower *dev : OutputPowerManager::devices())
            dev->pushMode(false);
    }
    if (IdleManager::instance())
        IdleManager::instance()->notifyActivity();
}

void Idle::suppress()
{
    Idle *self = instance();
    ++self->m_suppression;
    if (self->m_suppression == 1) {
        if (self->m_blanked)
            activity();
        if (IdleManager::instance())
            IdleManager::instance()->notifyActivity();
        qInfo() << "Idle: inhibition active";
    }
}

void Idle::release()
{
    Idle *self = instance();
    if (self->m_suppression > 0 && --self->m_suppression == 0) {
        self->m_idleTimer.restart();
        self->m_autoBlank->start();
        if (IdleManager::instance())
            IdleManager::instance()->notifyActivity();
        qInfo() << "Idle: inhibition released";
    }
}

bool Idle::suppressed()
{
    return instance()->m_suppression > 0;
}

quint64 Idle::idleMs()
{
    return quint64(instance()->m_idleTimer.elapsed());
}

void Idle::setBlanked(bool value)
{
    if (m_blanked == value)
        return;
    m_blanked = value;
    emit blankedChanged();
}

void Idle::wake()
{
    activity();
}

class IdleActivityFilter : public QObject
{
public:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        Q_UNUSED(obj)
        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
            Idle::activity();
            break;
        default:
            break;
        }
        return false;
    }
};

void Idle::registerTypes()
{
    static IdleActivityFilter filter;
    if (QGuiApplication::instance())
        QGuiApplication::instance()->installEventFilter(&filter);
}

QObject *Idle::create(QQmlEngine *engine, QJSEngine *js)
{
    Q_UNUSED(engine)
    Q_UNUSED(js)
    auto *self = instance();
    QQmlEngine::setObjectOwnership(self, QQmlEngine::CppOwnership);
    return self;
}

IdleNotification::IdleNotification(struct ::wl_client *client, uint32_t id, uint32_t version,
                                   uint32_t timeout)
{
    init(client, id, int(version));
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(int(timeout));
    connect(m_timer, &QTimer::timeout, this, [this] {
        if (Idle::suppressed()) {
            m_timer->start();
            return;
        }
        if (!m_idled) {
            m_idled = true;
            send_idled();
        }
    });
    armTimer();
}

IdleNotification::~IdleNotification() = default;

void IdleNotification::armTimer()
{
    m_timer->start();
}

void IdleNotification::onActivity()
{
    if (m_idled) {
        m_idled = false;
        send_resumed();
    }
    armTimer();
}

void IdleNotification::ext_idle_notification_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void IdleNotification::ext_idle_notification_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    deleteLater();
}

IdleManager *IdleManager::s_instance = nullptr;

IdleManager::IdleManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<IdleManager>()
{
    Q_UNUSED(parent)
    s_instance = this;
}

IdleManager::~IdleManager()
{
    if (s_instance == this)
        s_instance = nullptr;
}

void IdleManager::initialize()
{
    QWaylandCompositorExtensionTemplate<IdleManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    qInfo() << "IdleManager: ext_idle_notify_v1 ready";
}

void IdleManager::notifyActivity()
{
}

void IdleManager::ext_idle_notifier_v1_get_idle_notification(Resource *resource, uint32_t id,
                                                             uint32_t timeout, ::wl_resource *seat)
{
    Q_UNUSED(seat)
    auto *notification = new IdleNotification(resource->client(), id,
                                              wl_resource_get_version(resource->handle), timeout);
    connect(Idle::instance(), &Idle::activityOccurred, notification, &IdleNotification::onActivity);
}

QList<OutputPower *> OutputPowerManager::s_devices;

OutputPower::OutputPower(struct ::wl_client *client, uint32_t id, uint32_t version)
{
    init(client, id, int(version));
    OutputPowerManager::devices().append(this);
    pushMode(Idle::instance()->blanked());
}

OutputPower::~OutputPower()
{
    OutputPowerManager::devices().removeAll(this);
}

void OutputPower::pushMode(bool blanked)
{
    send_mode(blanked ? 0 : 1);
}

void OutputPower::zwlr_output_power_v1_set_mode(Resource *, uint32_t mode)
{
    if (mode == 0) {
        Idle::instance()->setBlanked(true);
        for (OutputPower *dev : OutputPowerManager::devices())
            dev->pushMode(true);
        return;
    }
    Idle::activity();
}

void OutputPower::zwlr_output_power_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void OutputPower::zwlr_output_power_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource)
    deleteLater();
}

OutputPowerManager::OutputPowerManager(QObject *parent)
    : QWaylandCompositorExtensionTemplate<OutputPowerManager>()
{
    Q_UNUSED(parent)
}

OutputPowerManager::~OutputPowerManager() = default;

void OutputPowerManager::initialize()
{
    QWaylandCompositorExtensionTemplate<OutputPowerManager>::initialize();
    auto *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor)
        return;
    init(compositor->display(), 1);
    qInfo() << "OutputPowerManager: wlr_output_power_management_v1 ready";
}

void OutputPowerManager::zwlr_output_power_manager_v1_get_output_power(Resource *resource, uint32_t id,
                                                                      ::wl_resource *output)
{
    Q_UNUSED(output)
    new OutputPower(resource->client(), id, wl_resource_get_version(resource->handle));
}

#include "idle.moc"
