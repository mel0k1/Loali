#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QQuickImageProvider>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtDBus/QDBusContext>
#include <qqml.h>

class QDBusServiceWatcher;

class SniRelay : public QObject
{
    Q_OBJECT

public:
    explicit SniRelay(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    void changed();
};

class Tray : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    enum Roles {
        UidRole = Qt::UserRole + 1,
        TitleRole,
        IconNameRole,
        IconSerialRole,
        StatusRole,
        ToolTipRole,
        MenuPathRole
    };

    static Tray *create(QQmlEngine *engine, QJSEngine *jsEngine);
    static Tray *instance() { return s_instance; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    bool available() const { return m_available; }

    Q_INVOKABLE void activate(int row, int x, int y) const;
    Q_INVOKABLE void secondaryActivate(int row, int x, int y) const;
    Q_INVOKABLE void contextMenu(int row, int x, int y) const;
    Q_INVOKABLE QVariantMap itemInfo(int row) const;

    QImage iconImage(const QString &key) const;

    void addItem(const QString &bus, const QString &path);
    void removeItem(const QString &key);
    void serviceGone(const QString &service);
    void setAvailable(bool value);

signals:
    void countChanged();

private slots:
    void fetchFinished();

private:
    struct Item {
        QString bus;
        QString path;
        QString title;
        QString status;
        QString iconName;
        QString toolTip;
        QString menuPath;
        QString menuService;
        QImage icon;
        int serial = 0;
        QDBusServiceWatcher *watcher = nullptr;
        SniRelay *relay = nullptr;
    };

    explicit Tray(QObject *parent = nullptr);

    void fetchProps(const QString &key);
    void applyProps(const QString &key, const QVariantMap &props);
    void itemCall(const Item &item, const QString &method, int x, int y) const;
    void watchItem(Item &item, const QString &key);

    QString keyAt(int row) const;
    Item *itemAt(const QString &key);

    static QImage decodePixmap(const QVariant &value);

    static Tray *s_instance;
    QStringList m_order;
    QHash<QString, Item> m_items;
    bool m_available = false;
};

class SniImageProvider : public QQuickImageProvider
{
public:
    SniImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

class StatusNotifierWatcher : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
    Q_PROPERTY(int ProtocolVersion READ protocolVersion CONSTANT)
    Q_PROPERTY(bool IsStatusNotifierHostRegistered READ hostRegistered NOTIFY hostChanged)
    Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ registeredItems NOTIFY itemsChanged)

public:
    explicit StatusNotifierWatcher(Tray *tray, QObject *parent = nullptr);

    void start();

    int protocolVersion() const { return 0; }
    bool hostRegistered() const { return m_host; }
    QStringList registeredItems() const;

public slots:
    void RegisterStatusNotifierItem(const QString &service);
    void RegisterStatusNotifierHost(const QString &service);

signals:
    void hostChanged();
    void itemsChanged();

private:
    Tray *m_tray = nullptr;
    bool m_host = false;
};
