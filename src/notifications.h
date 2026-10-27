#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QtDBus/QDBusContext>
#include <qqml.h>

class Notifications : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        AppNameRole,
        AppIconRole,
        PreviewRole,
        SummaryRole,
        BodyRole,
        UrgencyRole,
        ActionsRole
    };

    static Notifications *create(QQmlEngine *engine, QJSEngine *jsEngine);
    static Notifications *instance() { return s_instance; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void dismiss(int row);
    Q_INVOKABLE void invokeAction(int row, const QString &key);
    Q_INVOKABLE void clear();

    void expire(uint id, uint reason);

    uint notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int timeout);

signals:
    void countChanged();
    void notificationClosed(uint id, uint reason);
    void actionInvoked(uint id, const QString &actionKey);

private:
    struct Note {
        uint id = 0;
        QString appName;
        QString appIcon;
        QString preview;
        QString summary;
        QString body;
        int urgency = 1;
        QVariantMap actions;
        QTimer *timer = nullptr;
    };

    explicit Notifications(QObject *parent = nullptr);

    void schedule(Note &note, int timeout);

    static Notifications *s_instance;
    QList<Note> m_notes;
    uint m_nextId = 1;
};

class NotificationsDaemon : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit NotificationsDaemon(Notifications *model, QObject *parent = nullptr);

    void start();

public slots:
    QStringList GetCapabilities();
    void GetServerInformation(QString &name, QString &vendor, QString &version);
    uint Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int timeout);
    void CloseNotification(uint id);

signals:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

private:
    Notifications *m_model = nullptr;
};
