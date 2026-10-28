#include "portals.h"

#ifdef HAVE_PIPEWIRE
#include "pwcast.h"
#endif

#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>

namespace {

QQuickWindow *compositorWindow()
{
    const QList<QWindow *> windows = QGuiApplication::topLevelWindows();
    for (QWindow *w : windows) {
        if (auto *quick = qobject_cast<QQuickWindow *>(w))
            return quick;
    }
    return nullptr;
}

QString lastShotPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString base = dir.isEmpty() ? QDir::tempPath() : dir;
    QDir().mkpath(base);
    return base + QStringLiteral("/loali-portal-%1.png")
        .arg(QDateTime::currentMSecsSinceEpoch());
}

}

namespace {

struct PortalStream
{
    uint nodeId = 0;
    QVariantMap props;
};

}

Q_DECLARE_METATYPE(PortalStream)

QDBusArgument &operator<<(QDBusArgument &arg, const PortalStream &s)
{
    arg.beginStructure();
    arg << s.nodeId;
    arg << s.props;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, PortalStream &s)
{
    arg.beginStructure();
    arg >> s.nodeId;
    arg >> s.props;
    arg.endStructure();
    return arg;
}

class ScreenshotAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Screenshot")

public:
    explicit ScreenshotAdaptor(Portals *parent)
        : QDBusAbstractAdaptor(parent)
    {
    }

public slots:
    uint Screenshot(const QDBusObjectPath &handle, const QString &app_id,
                    const QString &parent_window, const QVariantMap &options,
                    QVariantMap &results)
    {
        Q_UNUSED(handle)
        Q_UNUSED(app_id)
        Q_UNUSED(parent_window)
        Q_UNUSED(options)
        auto *window = compositorWindow();
        if (!window) {
            qWarning() << "Portal Screenshot: no compositor window";
            return 2;
        }
        const QImage frame = window->grabWindow();
        const QString path = lastShotPath();
        if (frame.isNull() || !frame.save(path)) {
            qWarning() << "Portal Screenshot: capture failed" << frame.isNull();
            return 2;
        }
        qInfo() << "Portal Screenshot: saved" << path;
        results.insert(QStringLiteral("uri"), QUrl::fromLocalFile(path).toString());
        return 0;
    }
};

class ScreenCastAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.ScreenCast")

public:
    explicit ScreenCastAdaptor(Portals *parent)
        : QDBusAbstractAdaptor(parent)
    {
    }

public slots:
    uint CreateSession(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle,
                       const QString &app_id, const QVariantMap &options, QVariantMap &results)
    {
        Q_UNUSED(app_id)
        Q_UNUSED(options)
        Q_UNUSED(handle)
        results.insert(QStringLiteral("session_id"), session_handle.path().section(u'/', -1));
        return 0;
    }

    uint SelectSources(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle,
                       const QString &app_id, const QVariantMap &options, QVariantMap &results)
    {
        Q_UNUSED(handle)
        Q_UNUSED(session_handle)
        Q_UNUSED(app_id)
        Q_UNUSED(options)
        Q_UNUSED(results)
        return 0;
    }

    uint Start(const QDBusObjectPath &handle, const QDBusObjectPath &session_handle,
               const QString &app_id, const QString &parent_window,
               const QVariantMap &options, QVariantMap &results)
    {
        Q_UNUSED(handle)
        Q_UNUSED(session_handle)
        Q_UNUSED(app_id)
        Q_UNUSED(parent_window)
        Q_UNUSED(options)
#ifdef HAVE_PIPEWIRE
        auto *cast = PwScreenCast::instance();
        if (!cast->start())
            return 2;
        if (cast->nodeId() == 0) {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            QObject::connect(cast, &PwScreenCast::nodeReady, &loop, &QEventLoop::quit);
            timeout.start(3000);
            loop.exec();
        }
        if (cast->nodeId() == 0)
            return 2;
        static QTimer *frameTimer = nullptr;
        if (!frameTimer) {
            frameTimer = new QTimer(Portals::instance());
            QObject::connect(frameTimer, &QTimer::timeout, [cast] {
                auto *window = compositorWindow();
                if (window)
                    cast->setFrame(window->grabWindow());
            });
            frameTimer->start(33);
        }
        QVariantMap stream;
        stream.insert(QStringLiteral("node_id"), cast->nodeId());
        stream.insert(QStringLiteral("source_type"), uint(1));
        QList<PortalStream> streams;
        streams.append({ cast->nodeId(), stream });
        results.insert(QStringLiteral("streams"), QVariant::fromValue(streams));
        return 0;
#else
        Q_UNUSED(results)
        return 2;
#endif
    }
};

Portals *Portals::s_instance = nullptr;

Portals::Portals(QObject *parent)
    : QObject(parent)
{
    new ScreenshotAdaptor(this);
    new ScreenCastAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/portal/desktop"),
                                                 this);
}

bool Portals::create()
{
    QDBusConnection conn = QDBusConnection::sessionBus();
    if (!conn.registerService(QStringLiteral("org.freedesktop.impl.portal.desktop.loali"))) {
        qWarning() << "Portals: service registration failed:" << conn.lastError().message();
        return false;
    }
    s_instance = new Portals(QGuiApplication::instance());
    qInfo() << "Portals: org.freedesktop.impl.portal.desktop.loali ready";
    return true;
}

void Portals::registerTypes()
{
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<PortalStream>();
    qDBusRegisterMetaType<QList<PortalStream>>();
}

#include "portals.moc"
