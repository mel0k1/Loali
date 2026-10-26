#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <qqml.h>

class Apps : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList pinnedApps READ pinnedApps NOTIFY pinsChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IconRole,
        ExecRole,
        IdRole,
        CommentRole
    };

    static Apps *create(QQmlEngine *engine, QJSEngine *jsEngine);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList pinnedApps() const;

    Q_INVOKABLE QVariantList filterApps(const QString &query) const;
    Q_INVOKABLE void launch(int row) const;
    Q_INVOKABLE void launchById(const QString &id) const;
    Q_INVOKABLE bool isPinned(const QString &id) const;
    Q_INVOKABLE void togglePin(const QString &id);
    Q_INVOKABLE QString iconForAppId(const QString &appId) const;

signals:
    void pinsChanged();

private:
    struct Entry {
        QString id;
        QString name;
        QString icon;
        QString exec;
        QString comment;
        bool hidden = false;
    };

    explicit Apps(QObject *parent = nullptr);

    void scan();
    void loadPins();
    const Entry *find(const QString &id) const;
    void launchExec(const QString &exec) const;
    static QString normalizeId(QString id);

    static Apps *s_instance;
    QList<Entry> m_entries;
    QStringList m_pins;
};
