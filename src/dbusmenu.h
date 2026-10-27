#pragma once

#include <QDBusArgument>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <qqml.h>

class QDBusPendingCallWatcher;

struct DbusMenuNode
{
    int id = 0;
    QVariantMap props;
    QList<DbusMenuNode> children;
};

Q_DECLARE_METATYPE(DbusMenuNode)

const QDBusArgument &operator>>(const QDBusArgument &arg, DbusMenuNode &node);
QDBusArgument &operator<<(QDBusArgument &arg, const DbusMenuNode &node);

class TrayMenu : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString bus READ bus WRITE setBus NOTIFY targetChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY targetChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY levelChanged)
    Q_PROPERTY(bool canBack READ canBack NOTIFY levelChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit TrayMenu(QObject *parent = nullptr);

    QString bus() const { return m_bus; }
    void setBus(const QString &value);

    QString path() const { return m_path; }
    void setPath(const QString &value);

    QVariantList items() const { return m_items; }
    bool canBack() const { return m_stack.size() > 1; }
    bool busy() const { return m_pending > 0; }

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void enter(int id);
    Q_INVOKABLE void back();
    Q_INVOKABLE void trigger(int id);
    Q_INVOKABLE QString iconUrl(int id);

signals:
    void targetChanged();
    void levelChanged();
    void busyChanged();

private slots:
    void layoutFinished();
    void aboutToShowFinished();
    void layoutDirty();

private:
    void fetchLayout(int parentId);
    void subscribe();
    void flatten(const DbusMenuNode &node);
    void rebuild();
    QVariantMap entryFor(const DbusMenuNode &node) const;

    QString m_bus;
    QString m_path;
    QVariantList m_items;
    QList<int> m_stack;
    QHash<int, DbusMenuNode> m_nodes;
    QHash<int, QString> m_iconCache;
    bool m_subscribed = false;
    int m_seq = 0;
    int m_pending = 0;
};
