#include "tray.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QQmlEngine>

Tray *Tray::s_instance = nullptr;

Tray::Tray(QObject *parent)
    : QAbstractListModel(parent)
{
    auto *watcher = new StatusNotifierWatcher(this, this);
    watcher->start();
}

Tray *Tray::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)
    if (!s_instance)
        s_instance = new Tray();
    engine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

int Tray::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_order.size();
}

QVariant Tray::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= m_order.size())
        return QVariant();
    const Item &it = m_items.value(m_order.at(row));
    switch (role) {
    case UidRole:
        return m_order.at(row);
    case TitleRole:
        return it.title;
    case IconNameRole:
        return it.iconName;
    case IconSerialRole:
        return it.serial;
    case StatusRole:
        return it.status;
    case ToolTipRole:
        return it.toolTip.isEmpty() ? it.title : it.toolTip;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> Tray::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UidRole] = "uid";
    roles[TitleRole] = "title";
    roles[IconNameRole] = "iconName";
    roles[IconSerialRole] = "iconSerial";
    roles[StatusRole] = "status";
    roles[ToolTipRole] = "tooltip";
    return roles;
}

int Tray::count() const
{
    return m_order.size();
}

QString Tray::keyAt(int row) const
{
    return row >= 0 && row < m_order.size() ? m_order.at(row) : QString();
}

Tray::Item *Tray::itemAt(const QString &key)
{
    return m_items.contains(key) ? &m_items[key] : nullptr;
}

void Tray::addItem(const QString &bus, const QString &path)
{
    const QString key = bus + QStringLiteral("/") + path;
    if (m_items.contains(key))
        return;
    const int row = m_order.size();
    beginInsertRows(QModelIndex(), row, row);
    Item it;
    it.bus = bus;
    it.path = path;
    m_items.insert(key, it);
    m_order.append(key);
    endInsertRows();
    emit countChanged();
    watchItem(*itemAt(key), key);
    fetchProps(key);
}

void Tray::removeItem(const QString &key)
{
    const int row = m_order.indexOf(key);
    if (row < 0)
        return;
    if (Item *it = itemAt(key)) {
        if (it->watcher) {
            it->watcher->deleteLater();
            it->watcher = nullptr;
        }
        if (it->relay) {
            it->relay->deleteLater();
            it->relay = nullptr;
        }
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_items.remove(key);
    m_order.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void Tray::serviceGone(const QString &service)
{
    const QStringList keys = m_order;
    for (const QString &key : keys) {
        const Item *it = itemAt(key);
        if (it && it->bus == service)
            removeItem(key);
    }
}

void Tray::setAvailable(bool value)
{
    m_available = value;
}

void Tray::watchItem(Item &item, const QString &key)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    item.watcher = new QDBusServiceWatcher(item.bus, bus,
                                           QDBusServiceWatcher::WatchForUnregistration, this);
    connect(item.watcher, &QDBusServiceWatcher::serviceUnregistered,
            this, [this](const QString &service) { serviceGone(service); });

    auto *relay = new SniRelay(this);
    item.relay = relay;
    connect(relay, &SniRelay::changed, this, [this, key]() { fetchProps(key); });

    const QString iface = QStringLiteral("org.kde.StatusNotifierItem");
    bus.connect(item.bus, item.path, iface, QStringLiteral("NewIcon"), relay, SLOT(changed()));
    bus.connect(item.bus, item.path, iface, QStringLiteral("NewTitle"), relay, SLOT(changed()));
    bus.connect(item.bus, item.path, iface, QStringLiteral("NewStatus"), relay, SLOT(changed()));
    bus.connect(item.bus, item.path, iface, QStringLiteral("NewToolTip"), relay, SLOT(changed()));
    bus.connect(item.bus, item.path, iface, QStringLiteral("NewAttentionIcon"), relay, SLOT(changed()));
    bus.connect(item.bus, item.path, iface, QStringLiteral("NewOverlayIcon"), relay, SLOT(changed()));
    bus.connect(item.bus, item.path, QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("PropertiesChanged"), relay, SLOT(changed()));
}

void Tray::fetchProps(const QString &key)
{
    const Item *it = itemAt(key);
    if (!it)
        return;
    QDBusMessage msg = QDBusMessage::createMethodCall(
        it->bus, it->path, QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("GetAll"));
    msg << QStringLiteral("org.kde.StatusNotifierItem");
    QDBusPendingReply<QVariantMap> reply = QDBusConnection::sessionBus().asyncCall(msg);
    auto *call = new QDBusPendingCallWatcher(reply, this);
    call->setProperty("key", key);
    connect(call, &QDBusPendingCallWatcher::finished, this, &Tray::fetchFinished);
}

void Tray::fetchFinished()
{
    auto *call = static_cast<QDBusPendingCallWatcher *>(sender());
    call->deleteLater();
    QDBusPendingReply<QVariantMap> reply = *call;
    const QString key = call->property("key").toString();
    if (!reply.isValid()) {
        removeItem(key);
        return;
    }
    if (!itemAt(key))
        return;
    applyProps(key, reply.value());
}

void Tray::applyProps(const QString &key, const QVariantMap &props)
{
    Item *it = itemAt(key);
    if (!it)
        return;
    it->title = props.value(QStringLiteral("Title")).toString();
    it->status = props.value(QStringLiteral("Status")).toString();
    it->iconName = props.value(QStringLiteral("IconName")).toString();
    const QVariantMap tip = props.value(QStringLiteral("ToolTip")).toMap();
    it->toolTip = tip.value(QStringLiteral("title")).toString();
    if (it->iconName.isEmpty())
        it->icon = decodePixmap(props.value(QStringLiteral("IconPixmap")));
    else
        it->icon = QImage();
    ++it->serial;
    const int row = m_order.indexOf(key);
    if (row >= 0)
        emit dataChanged(index(row, 0), index(row, 0));
}

QImage Tray::decodePixmap(const QVariant &value)
{
    struct Px {
        int w;
        int h;
        QByteArray bytes;
    };
    QList<Px> list;
    const int type = value.userType();
    if (type == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = value.value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            arg.beginStructure();
            qint32 w = 0;
            qint32 h = 0;
            QByteArray bytes;
            arg >> w >> h >> bytes;
            arg.endStructure();
            if (w > 0 && h > 0 && bytes.size() >= w * h * 4)
                list.append({int(w), int(h), bytes});
        }
        arg.endArray();
    } else if (type == QMetaType::QVariantList) {
        const QVariantList arr = value.toList();
        for (const QVariant &e : arr) {
            const QVariantList t = e.toList();
            if (t.size() < 3)
                continue;
            const int w = t.at(0).toInt();
            const int h = t.at(1).toInt();
            const QByteArray bytes = t.at(2).toByteArray();
            if (w > 0 && h > 0 && bytes.size() >= w * h * 4)
                list.append({w, h, bytes});
        }
    }
    const Px *best = nullptr;
    int bestArea = 0;
    for (const Px &p : list) {
        if (p.w * p.h > bestArea) {
            bestArea = p.w * p.h;
            best = &p;
        }
    }
    if (!best)
        return QImage();
    QImage img(best->w, best->h, QImage::Format_ARGB32);
    const uchar *src = reinterpret_cast<const uchar *>(best->bytes.constData());
    for (int y = 0; y < best->h; ++y) {
        for (int x = 0; x < best->w; ++x) {
            const int i = (y * best->w + x) * 4;
            img.setPixel(x, y, qRgba(src[i + 1], src[i + 2], src[i + 3], src[i]));
        }
    }
    return img;
}

QImage Tray::iconImage(const QString &key) const
{
    return m_items.contains(key) ? m_items.value(key).icon : QImage();
}

void Tray::itemCall(const Item &item, const QString &method, int x, int y) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        item.bus, item.path, QStringLiteral("org.kde.StatusNotifierItem"), method);
    msg << x << y;
    QDBusConnection::sessionBus().asyncCall(msg);
}

void Tray::activate(int row, int x, int y) const
{
    const QString key = keyAt(row);
    const Item *it = const_cast<Tray *>(this)->itemAt(key);
    if (it)
        itemCall(*it, QStringLiteral("Activate"), x, y);
}

void Tray::secondaryActivate(int row, int x, int y) const
{
    const QString key = keyAt(row);
    const Item *it = const_cast<Tray *>(this)->itemAt(key);
    if (it)
        itemCall(*it, QStringLiteral("SecondaryActivate"), x, y);
}

void Tray::contextMenu(int row, int x, int y) const
{
    const QString key = keyAt(row);
    const Item *it = const_cast<Tray *>(this)->itemAt(key);
    if (it)
        itemCall(*it, QStringLiteral("ContextMenu"), x, y);
}

StatusNotifierWatcher::StatusNotifierWatcher(Tray *tray, QObject *parent)
    : QObject(parent)
    , m_tray(tray)
{
}

void StatusNotifierWatcher::start()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        m_tray->setAvailable(false);
        return;
    }
    if (!bus.registerService(QStringLiteral("org.kde.StatusNotifierWatcher"))) {
        m_tray->setAvailable(false);
        return;
    }
    bus.registerObject(QStringLiteral("/StatusNotifierWatcher"), this,
                       QDBusConnection::ExportAllContents);
    m_tray->setAvailable(true);
    RegisterStatusNotifierHost(QStringLiteral("org.kde.StatusNotifierWatcher"));
}

void StatusNotifierWatcher::RegisterStatusNotifierItem(const QString &service)
{
    if (!calledFromDBus())
        return;
    QString name = service;
    QString path = QStringLiteral("/StatusNotifierItem");
    if (name.contains(QLatin1Char('/'))) {
        path = name.section(QLatin1Char('/'), 1);
        name = name.section(QLatin1Char('/'), 0, 0);
    }
    if (name.isEmpty())
        name = message().service();
    if (name.isEmpty())
        return;
    m_tray->addItem(name, path);
    emit itemsChanged();
}

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString &service)
{
    Q_UNUSED(service)
    if (!m_host) {
        m_host = true;
        emit hostChanged();
    }
}

QStringList StatusNotifierWatcher::registeredItems() const
{
    QStringList res;
    for (int i = 0; i < m_tray->rowCount(); ++i)
        res.append(m_tray->data(m_tray->index(i, 0), Tray::UidRole).toString());
    return res;
}

SniImageProvider::SniImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage SniImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString key = id.section(QLatin1Char('?'), 0, 0);
    QImage img = Tray::instance() ? Tray::instance()->iconImage(key) : QImage();
    if (!img.isNull() && requestedSize.width() > 0 && requestedSize.height() > 0
        && requestedSize != img.size())
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (size)
        *size = img.size();
    return img;
}
