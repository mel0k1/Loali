#include "notifications.h"

#include <QBuffer>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QImage>
#include <QQmlEngine>
#include <algorithm>

Notifications *Notifications::s_instance = nullptr;

namespace {

QImage decodePixmap(const QVariant &value)
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

QString pixmapDataUrl(const QVariant &value)
{
    const QImage img = decodePixmap(value);
    if (img.isNull())
        return QString();
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(bytes.toBase64());
}

}

Notifications::Notifications(QObject *parent)
    : QAbstractListModel(parent)
{
}

Notifications *Notifications::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)
    if (!s_instance)
        s_instance = new Notifications();
    engine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

int Notifications::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_notes.size();
}

QVariant Notifications::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= m_notes.size())
        return QVariant();
    const Note &n = m_notes.at(row);
    switch (role) {
    case IdRole:
        return n.id;
    case AppNameRole:
        return n.appName;
    case AppIconRole:
        return n.appIcon;
    case PreviewRole:
        return n.preview;
    case SummaryRole:
        return n.summary;
    case BodyRole:
        return n.body;
    case UrgencyRole:
        return n.urgency;
    case ActionsRole:
        return n.actions;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> Notifications::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "nid";
    roles[AppNameRole] = "appName";
    roles[AppIconRole] = "appIcon";
    roles[PreviewRole] = "preview";
    roles[SummaryRole] = "summary";
    roles[BodyRole] = "body";
    roles[UrgencyRole] = "urgency";
    roles[ActionsRole] = "actions";
    return roles;
}

int Notifications::count() const
{
    return m_notes.size();
}

void Notifications::dismiss(int row)
{
    if (row < 0 || row >= m_notes.size())
        return;
    expire(m_notes.at(row).id, 2);
}

void Notifications::invokeAction(int row, const QString &key)
{
    if (row < 0 || row >= m_notes.size())
        return;
    emit actionInvoked(m_notes.at(row).id, key);
    expire(m_notes.at(row).id, 1);
}

void Notifications::clear()
{
    const QList<Note> notes = m_notes;
    for (const Note &n : notes)
        expire(n.id, 3);
}

uint Notifications::notify(const QString &appName, uint replacesId, const QString &appIcon,
                           const QString &summary, const QString &body, const QStringList &actions,
                           const QVariantMap &hints, int timeout)
{
    Note note;
    note.appName = appName;
    note.appIcon = appIcon;
    note.summary = summary;
    note.body = body;
    note.urgency = hints.value(QStringLiteral("urgency"), 1).toInt();

    const QString imagePath = hints.value(QStringLiteral("image-path"))
                                  .toString();
    if (imagePath.startsWith(QStringLiteral("/")))
        note.preview = QStringLiteral("file:") + imagePath;

    QVariantMap actionMap;
    for (int i = 0; i + 1 < actions.size(); i += 2) {
        if (actions.at(i) != QStringLiteral("default"))
            actionMap.insert(actions.at(i), actions.at(i + 1));
    }
    note.actions = actionMap;

    if (note.preview.isEmpty()) {
        const QVariant data = hints.contains(QStringLiteral("image-data"))
                                  ? hints.value(QStringLiteral("image-data"))
                                  : hints.value(QStringLiteral("icon_data"));
        if (data.isValid())
            note.preview = pixmapDataUrl(data);
    }

    uint id = replacesId;
    int row = -1;
    if (replacesId > 0) {
        for (int i = 0; i < m_notes.size(); ++i) {
            if (m_notes.at(i).id == replacesId) {
                row = i;
                break;
            }
        }
    }
    if (row < 0) {
        id = m_nextId++;
        if (m_notes.size() >= 8)
            expire(m_notes.first().id, 1);
        beginInsertRows(QModelIndex(), 0, 0);
        note.id = id;
        m_notes.prepend(note);
        endInsertRows();
        emit countChanged();
        row = 0;
    } else {
        m_notes[row].appName = note.appName;
        m_notes[row].appIcon = note.appIcon;
        m_notes[row].preview = note.preview;
        m_notes[row].summary = note.summary;
        m_notes[row].body = note.body;
        m_notes[row].urgency = note.urgency;
        m_notes[row].actions = note.actions;
        emit dataChanged(index(row, 0), index(row, 0));
    }
    schedule(m_notes[row], timeout);
    return id;
}

void Notifications::schedule(Note &note, int timeout)
{
    if (note.timer) {
        note.timer->stop();
        note.timer->deleteLater();
        note.timer = nullptr;
    }
    if (note.urgency >= 2)
        return;
    int ms = timeout == 0 ? 0 : timeout < 0 ? 7000 : timeout;
    if (ms <= 0)
        return;
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    const uint id = note.id;
    connect(timer, &QTimer::timeout, this, [this, id]() { expire(id, 1); });
    timer->start(ms);
    note.timer = timer;
}

void Notifications::expire(uint id, uint reason)
{
    int row = -1;
    for (int i = 0; i < m_notes.size(); ++i) {
        if (m_notes.at(i).id == id) {
            row = i;
            break;
        }
    }
    if (row < 0)
        return;
    if (m_notes.at(row).timer) {
        m_notes.at(row).timer->stop();
        m_notes.at(row).timer->deleteLater();
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_notes.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit notificationClosed(id, reason);
}

NotificationsDaemon::NotificationsDaemon(Notifications *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
{
    connect(m_model, &Notifications::notificationClosed, this, &NotificationsDaemon::NotificationClosed);
    connect(m_model, &Notifications::actionInvoked, this, &NotificationsDaemon::ActionInvoked);
}

void NotificationsDaemon::start()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    bus.registerService(QStringLiteral("org.freedesktop.Notifications"));
    bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), this,
                       QDBusConnection::ExportAllContents);
}

QStringList NotificationsDaemon::GetCapabilities()
{
    return {QStringLiteral("actions"), QStringLiteral("body"), QStringLiteral("body-markup"),
            QStringLiteral("icon-static")};
}

void NotificationsDaemon::GetServerInformation(QString &name, QString &vendor, QString &version)
{
    name = QStringLiteral("Loali");
    vendor = QStringLiteral("Loali Project");
    version = QStringLiteral("0.2.0");
}

uint NotificationsDaemon::Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                                 const QString &summary, const QString &body,
                                 const QStringList &actions, const QVariantMap &hints, int timeout)
{
    return m_model->notify(app_name, replaces_id, app_icon, summary, body, actions, hints, timeout);
}

void NotificationsDaemon::CloseNotification(uint id)
{
    m_model->expire(id, 3);
}
