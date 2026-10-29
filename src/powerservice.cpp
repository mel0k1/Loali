#include "powerservice.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusAbstractAdaptor>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QTimer>

#include "idle.h"

PowerService *PowerService::s_instance = nullptr;

namespace {

class ScreenSaverAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")
public:
    explicit ScreenSaverAdaptor(PowerService *parent)
        : QDBusAbstractAdaptor(parent)
        , m_service(parent)
    {
    }

public slots:
    uint Inhibit(const QString &application_name, const QString &reason_for_inhibit)
    {
        return uint(m_service->inhibit(application_name, reason_for_inhibit));
    }

    void UnInhibit(uint cookie)
    {
        m_service->uninhibit(int(cookie));
    }

    void SimulateUserActivity()
    {
        m_service->simulateUserActivity();
    }

    uint GetSessionIdleTime()
    {
        return uint(m_service->idleTime());
    }

private:
    PowerService *m_service;
};

} // namespace

PowerService::PowerService(QObject *parent)
    : QObject(parent)
{
    probeLogind();
}

PowerService *PowerService::instance()
{
    if (!s_instance)
        s_instance = new PowerService(QGuiApplication::instance());
    return s_instance;
}

void PowerService::registerType()
{
    qmlRegisterSingletonType<PowerService>("Loali", 1, 0, "Power",
                                           [](QQmlEngine *engine, QJSEngine *js) -> QObject * {
                                               Q_UNUSED(js)
                                               PowerService *self = instance();
                                               engine->setObjectOwnership(self, QQmlEngine::CppOwnership);
                                               return self;
                                           });
}

void PowerService::start()
{
    PowerService *self = instance();
    auto bus = QDBusConnection::sessionBus();
    new ScreenSaverAdaptor(self);
    if (!bus.registerService(QStringLiteral("org.freedesktop.ScreenSaver")))
        qWarning() << "PowerService: register ScreenSaver failed:" << bus.lastError().message();
    if (!bus.registerObject(QStringLiteral("/org/freedesktop/ScreenSaver"), self))
        qWarning() << "PowerService: register object failed:" << bus.lastError().message();
    self->watchIdle();
    qInfo() << "PowerService: org.freedesktop.ScreenSaver ready";
}

void PowerService::watchIdle()
{
    connect(Idle::instance(), &Idle::blankedChanged, this, [this] {
        if (Idle::instance()->blanked()) {
            qInfo() << "PowerService: idle blank -> lock";
            emit lockRequested();
        }
    });
}

void PowerService::probeLogind()
{
    QDBusConnection sys = QDBusConnection::systemBus();
    m_logind = sys.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.login1"));
    if (!m_logind)
        return;
    const QStringList props = { QStringLiteral("CanSuspend"), QStringLiteral("CanPowerOff"),
                                QStringLiteral("CanReboot") };
    QDBusInterface manager(QStringLiteral("org.freedesktop.login1"),
                           QStringLiteral("/org/freedesktop/login1"),
                           QStringLiteral("org.freedesktop.DBus.Properties"), sys, this);
    for (const QString &p : props) {
        QDBusReply<QVariant> reply = manager.call(QStringLiteral("Get"),
                                                  QStringLiteral("org.freedesktop.login1.Manager"), p);
        const QString value = reply.isValid() ? reply.value().toString() : QString();
        const bool ok = (value == QLatin1String("yes"));
        if (p == QLatin1String("CanSuspend"))
            m_canSuspend = ok;
        else if (p == QLatin1String("CanPowerOff"))
            m_canPowerOff = ok;
        else
            m_canReboot = ok;
    }
    sys.connect(QStringLiteral("org.freedesktop.login1"),
                QStringLiteral("/org/freedesktop/login1"),
                QStringLiteral("org.freedesktop.login1.Manager"),
                QStringLiteral("PrepareForSleep"),
                this, SLOT(onPrepareForSleep(bool)));
    const QString selfSession = QStringLiteral("/org/freedesktop/login1/session/self");
    const bool lockWatch = sys.connect(QStringLiteral("org.freedesktop.login1"), selfSession,
                                       QStringLiteral("org.freedesktop.login1.Session"),
                                       QStringLiteral("Lock"),
                                       this, SIGNAL(lockRequested()));
    const bool unlockWatch = sys.connect(QStringLiteral("org.freedesktop.login1"), selfSession,
                                         QStringLiteral("org.freedesktop.login1.Session"),
                                         QStringLiteral("Unlock"),
                                         this, SIGNAL(unlockRequested()));
    if (lockWatch || unlockWatch)
        qInfo() << "PowerService: logind session lock signals connected";
    emit powerActionsChanged();
}

void PowerService::onPrepareForSleep(bool start)
{
    if (start) {
        qInfo() << "PowerService: prepare for sleep -> lock";
        emit lockRequested();
    } else {
        Idle::activity();
    }
}

bool PowerService::callLogind(const QString &method)
{
    if (!m_logind) {
        emit actionFailed(method, QStringLiteral("org.freedesktop.login1 is not available"));
        return false;
    }
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.login1"),
                                                       QStringLiteral("/org/freedesktop/login1"),
                                                       QStringLiteral("org.freedesktop.login1.Manager"),
                                                       method);
    call.setArguments({ false });
    QDBusPendingCall pending = QDBusConnection::systemBus().asyncCall(call);
    auto *watcher = new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, method](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError())
            emit actionFailed(method, reply.error().message());
        w->deleteLater();
    });
    return true;
}

int PowerService::inhibit(const QString &application, const QString &reason)
{
    const uint cookie = ++m_nextCookie;
    m_inhibits.insert(cookie, { application, reason });
    if (m_inhibits.size() == 1)
        Idle::suppress();
    emit inhibited(int(cookie), application, reason);
    qInfo() << "PowerService: inhibit" << cookie << application << reason;
    return int(cookie);
}

void PowerService::uninhibit(int cookie)
{
    if (!m_inhibits.remove(uint(cookie)))
        return;
    if (m_inhibits.isEmpty())
        Idle::release();
    emit uninhibited(cookie);
    qInfo() << "PowerService: uninhibit" << cookie;
}

void PowerService::simulateUserActivity()
{
    Idle::activity();
    emit activitySimulated();
}

quint64 PowerService::idleTime() const
{
    return Idle::idleMs();
}

bool PowerService::suspend()
{
    return callLogind(QStringLiteral("Suspend"));
}

bool PowerService::powerOff()
{
    return callLogind(QStringLiteral("PowerOff"));
}

bool PowerService::reboot()
{
    return callLogind(QStringLiteral("Reboot"));
}

void PowerService::lock()
{
    emit lockRequested();
}

void PowerService::logout()
{
    emit logoutRequested();
}

#include "powerservice.moc"
