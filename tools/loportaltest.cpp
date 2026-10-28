#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDebug>
#include <QTimer>
#include <QVariantMap>


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

namespace {

uint streamNodeId(const QVariant &v)
{
    if (v.userType() == qMetaTypeId<QDBusArgument>()) {
        const QList<PortalStream> streams =
            qdbus_cast<QList<PortalStream>>(v.value<QDBusArgument>());
        return streams.isEmpty() ? uint(0) : streams.first().nodeId;
    }
    const QVariantList l = v.toList();
    if (!l.isEmpty()) {
        const QVariantMap m = l.first().toMap();
        return m.value(QStringLiteral("node_id")).toUInt();
    }
    return v.toMap().value(QStringLiteral("node_id")).toUInt();
}

}


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qDBusRegisterMetaType<PortalStream>();
    qDBusRegisterMetaType<QList<PortalStream>>();
    const QString mode = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("screenshot");
    const QString iface = mode == QStringLiteral("screencast")
        ? QStringLiteral("org.freedesktop.impl.portal.ScreenCast")
        : QStringLiteral("org.freedesktop.impl.portal.Screenshot");

    QTimer::singleShot(15000, [&] {
        qWarning() << "portal test timeout";
        QCoreApplication::exit(3);
    });

    const auto invoke = [&iface](const QString &method, const QList<QVariant> &args,
                                 uint &code, QVariantMap &results) -> bool {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.impl.portal.desktop.loali"),
            QStringLiteral("/org/freedesktop/portal/desktop"), iface, method);
        msg.setArguments(args);
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 12000);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << method << "failed:" << reply.errorMessage();
            return false;
        }
        const QVariantList out = reply.arguments();
        fflush(stdout);
        if (out.isEmpty())
            return false;
        code = out.at(0).toUInt();
        if (out.size() > 1) {
            {
                QVariant v = out.at(1);
                if (v.userType() == qMetaTypeId<QDBusArgument>()) {
                    QDBusArgument arg = v.value<QDBusArgument>();
                    QVariantMap m;
                    arg.beginMap();
                    while (!arg.atEnd()) {
                        arg.beginMapEntry();
                        QVariant key = arg.asVariant();
                        QVariant val = arg.asVariant();
                        if (val.userType() == qMetaTypeId<QDBusVariant>())
                            val = val.value<QDBusVariant>().variant();
                        arg.endMapEntry();
                        m.insert(key.toString(), val);
                    }
                    arg.endMap();
                    v = m;
                }
                results = v.toMap();
            }
            for (auto it = results.cbegin(); it != results.cend(); ++it)
                printf("DBG key=%s vtype=%s\n", qPrintable(it.key()), it.value().typeName());
            fflush(stdout);
        }
        return true;
    };

    uint code = 0;
    QVariantMap results;

    if (mode == QStringLiteral("screencast")) {
        if (!invoke(QStringLiteral("CreateSession"),
                    { QVariant::fromValue(QDBusObjectPath(QStringLiteral("/h"))),
                      QVariant::fromValue(QDBusObjectPath(QStringLiteral("/s"))),
                      QStringLiteral("lotest"), QVariantMap() }, code, results))
            return 1;
        if (code != 0)
            return 1;
        results.clear();
        if (!invoke(QStringLiteral("Start"),
                    { QVariant::fromValue(QDBusObjectPath(QStringLiteral("/h"))),
                      QVariant::fromValue(QDBusObjectPath(QStringLiteral("/s"))),
                      QStringLiteral("lotest"), QString(), QVariantMap() }, code, results))
            return 1;
        if (code != 0) {
            qWarning() << "Start response code" << code;
            return 1;
        }
        const uint nodeId = streamNodeId(results.value(QStringLiteral("streams")));
        if (nodeId == 0)
            qWarning() << "node id not decoded (protocol-wise ok)";
        printf("PW_NODE=%u\n", nodeId);
        fflush(stdout);
        QTimer::singleShot(3000, &app, QCoreApplication::quit);
        return 0;
    }

    if (!invoke(QStringLiteral("Screenshot"),
                { QVariant::fromValue(QDBusObjectPath(QStringLiteral("/h"))),
                  QStringLiteral("lotest"), QString(), QVariantMap() }, code, results))
        return 1;
    printf("CODE=%u KEYS=%d\n", code, results.size());
    if (code != 0)
        return 1;
    const QString uri = results.value(QStringLiteral("uri")).toString();
    printf("SHOT_URI=%s\n", qPrintable(uri));
    fflush(stdout);
    return uri.startsWith(QStringLiteral("file://")) ? 0 : 2;
}
