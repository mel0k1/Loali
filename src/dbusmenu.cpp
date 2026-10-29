#include "dbusmenu.h"

#include <QBuffer>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDateTime>
#include <QImage>

namespace {

QString cleanLabel(const QString &raw)
{
    QString s = raw;
    s.replace(QStringLiteral("&&"), QStringLiteral("\x01"));
    s.replace(QStringLiteral("__"), QStringLiteral("\x02"));
    const int idx = s.indexOf(QLatin1Char('_'));
    if (idx >= 0 && idx + 1 < s.size() && s.at(idx + 1) != QLatin1Char('_'))
        s.remove(idx, 1);
    s.replace(QLatin1Char('\x01'), QLatin1Char('&'));
    s.replace(QLatin1Char('\x02'), QLatin1Char('_'));
    return s.trimmed();
}

QByteArray variantBytes(const QVariant &value)
{
    if (value.userType() == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = value.value<QDBusArgument>();
        QByteArray bytes;
        arg >> bytes;
        return bytes;
    }
    return value.toByteArray();
}

}

const QDBusArgument &operator>>(const QDBusArgument &arg, DbusMenuNode &node)
{
    arg.beginStructure();
    arg >> node.id;
    node.props.clear();
    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QDBusVariant val;
        arg.beginMapEntry();
        arg >> key >> val;
        arg.endMapEntry();
        node.props.insert(key, val.variant());
    }
    arg.endMap();
    node.children.clear();
    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant childVar;
        arg >> childVar;
        DbusMenuNode child;
        childVar.variant().value<QDBusArgument>() >> child;
        node.children.append(child);
    }
    arg.endArray();
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const DbusMenuNode &node)
{
    arg.beginStructure();
    arg << node.id;
    arg.beginMap(QMetaType::QString, QMetaType::QVariant);
    for (auto it = node.props.constBegin(); it != node.props.constEnd(); ++it) {
        arg.beginMapEntry();
        arg << it.key() << QDBusVariant(it.value());
        arg.endMapEntry();
    }
    arg.endMap();
    arg.beginArray(QMetaType::QVariant);
    for (const DbusMenuNode &child : node.children)
        arg << QDBusVariant(QVariant::fromValue(child));
    arg.endArray();
    arg.endStructure();
    return arg;
}

TrayMenu::TrayMenu(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<DbusMenuNode>();
}

void TrayMenu::setBus(const QString &value)
{
    if (m_bus == value)
        return;
    m_bus = value;
    m_subscribed = false;
    m_nodes.clear();
    m_stack.clear();
    m_iconCache.clear();
    emit targetChanged();
}

void TrayMenu::setPath(const QString &value)
{
    if (m_path == value)
        return;
    m_path = value;
    m_subscribed = false;
    m_nodes.clear();
    m_stack.clear();
    m_iconCache.clear();
    emit targetChanged();
}

void TrayMenu::subscribe()
{
    if (m_subscribed || m_bus.isEmpty() || m_path.isEmpty())
        return;
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    bus.connect(m_bus, m_path, QStringLiteral("com.canonical.dbusmenu"),
                QStringLiteral("LayoutUpdated"), this, SLOT(layoutDirty()));
    bus.connect(m_bus, m_path, QStringLiteral("com.canonical.dbusmenu"),
                QStringLiteral("ItemsPropertiesUpdated"), this, SLOT(layoutDirty()));
    m_subscribed = true;
}

void TrayMenu::open()
{
    if (m_bus.isEmpty() || m_path.isEmpty())
        return;
    subscribe();
    m_seq++;
    m_stack.clear();
    m_stack.append(0);
    fetchLayout(0);
}

void TrayMenu::close()
{
    m_seq++;
    m_stack.clear();
    if (m_pending != 0) {
        m_pending = 0;
        emit busyChanged();
    }
}

void TrayMenu::enter(int id)
{
    const DbusMenuNode node = m_nodes.value(id);
    if (node.children.isEmpty())
        return;
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_bus, m_path, QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("AboutToShow"));
    msg << id;
    QDBusPendingCallWatcher *call = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);
    call->setProperty("seq", m_seq);
    call->setProperty("nodeId", id);
    connect(call, &QDBusPendingCallWatcher::finished, this, &TrayMenu::aboutToShowFinished);
}

void TrayMenu::aboutToShowFinished()
{
    auto *call = static_cast<QDBusPendingCallWatcher *>(sender());
    call->deleteLater();
    if (call->property("seq").toInt() != m_seq)
        return;
    QDBusPendingReply<bool> reply = *call;
    const int id = call->property("nodeId").toInt();
    if (!m_stack.contains(id))
        return;
    if (reply.isValid() && reply.value())
        fetchLayout(id);
    if (m_stack.last() != id) {
        m_stack.append(id);
        rebuild();
    }
}

void TrayMenu::back()
{
    if (m_stack.size() <= 1)
        return;
    m_seq++;
    m_stack.removeLast();
    fetchLayout(m_stack.last());
}

void TrayMenu::trigger(int id)
{
    if (!m_nodes.contains(id))
        return;
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_bus, m_path, QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("Event"));
    msg << id << QStringLiteral("clicked")
        << QVariant::fromValue(QDBusVariant(QString()))
        << uint(QDateTime::currentSecsSinceEpoch());
    QDBusConnection::sessionBus().asyncCall(msg);
}

void TrayMenu::fetchLayout(int parentId)
{
    if (m_bus.isEmpty() || m_path.isEmpty())
        return;
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_bus, m_path, QStringLiteral("com.canonical.dbusmenu"), QStringLiteral("GetLayout"));
    msg << parentId << 2 << QStringList();
    QDBusPendingCallWatcher *call = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);
    call->setProperty("seq", m_seq);
    call->setProperty("parentId", parentId);
    m_pending++;
    if (m_pending == 1)
        emit busyChanged();
    connect(call, &QDBusPendingCallWatcher::finished, this, &TrayMenu::layoutFinished);
}

void TrayMenu::layoutDirty()
{
    if (m_stack.isEmpty())
        return;
    m_seq++;
    fetchLayout(m_stack.last());
}

void TrayMenu::layoutFinished()
{
    auto *call = static_cast<QDBusPendingCallWatcher *>(sender());
    call->deleteLater();
    if (m_pending > 0) {
        m_pending--;
        if (m_pending == 0)
            emit busyChanged();
    }
    if (call->property("seq").toInt() != m_seq || m_stack.isEmpty())
        return;
    QDBusPendingReply<uint, DbusMenuNode> reply = *call;
    if (!reply.isValid() || !m_stack.contains(call->property("parentId").toInt()))
        return;
    m_nodes.clear();
    m_iconCache.clear();
    flatten(reply.argumentAt(1).value<DbusMenuNode>());
    rebuild();
}

void TrayMenu::flatten(const DbusMenuNode &node)
{
    m_nodes.insert(node.id, node);
    for (const DbusMenuNode &child : node.children)
        flatten(child);
}

QVariantMap TrayMenu::entryFor(const DbusMenuNode &node) const
{
    const QString type = node.props.value(QStringLiteral("type")).toString();
    QVariantMap entry;
    entry.insert(QStringLiteral("id"), node.id);
    entry.insert(QStringLiteral("sep"), type == QStringLiteral("separator"));
    entry.insert(QStringLiteral("label"), cleanLabel(node.props.value(QStringLiteral("label")).toString()));
    entry.insert(QStringLiteral("enabled"), node.props.value(QStringLiteral("enabled"), true).toBool());
    entry.insert(QStringLiteral("iconName"), node.props.value(QStringLiteral("icon-name")).toString());
    entry.insert(QStringLiteral("hasIcon"), !variantBytes(node.props.value(QStringLiteral("icon-data"))).isEmpty());
    entry.insert(QStringLiteral("toggleType"), node.props.value(QStringLiteral("toggle-type")).toString());
    entry.insert(QStringLiteral("toggleState"), node.props.value(QStringLiteral("toggle-state"), 0).toInt());
    entry.insert(QStringLiteral("hasChildren"),
                 node.props.value(QStringLiteral("children-display")).toString() == QStringLiteral("submenu")
                     && !node.children.isEmpty());
    return entry;
}

void TrayMenu::rebuild()
{
    m_items.clear();
    if (!m_stack.isEmpty()) {
        const DbusMenuNode top = m_nodes.value(m_stack.last());
        for (const DbusMenuNode &child : top.children) {
            if (child.props.value(QStringLiteral("visible"), true).toBool())
                m_items.append(entryFor(child));
        }
    }
    emit levelChanged();
}

QString TrayMenu::iconUrl(int id)
{
    if (m_iconCache.contains(id))
        return m_iconCache.value(id);
    const DbusMenuNode node = m_nodes.value(id);
    const QByteArray bytes = variantBytes(node.props.value(QStringLiteral("icon-data")));
    QImage img;
    if (!bytes.isEmpty())
        img.loadFromData(bytes);
    if (img.isNull()) {
        m_iconCache.insert(id, QString());
        return QString();
    }
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    const QString url = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
    m_iconCache.insert(id, url);
    return url;
}
