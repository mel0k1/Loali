#pragma once

#include <QColor>
#include <QObject>
#include <qqml.h>

class LoaliSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_NAMED_ELEMENT(Settings)
    Q_PROPERTY(QString wallpaper READ wallpaper WRITE setWallpaper NOTIFY wallpaperChanged)
    Q_PROPERTY(qreal dim READ dim WRITE setDim NOTIFY dimChanged)
    Q_PROPERTY(QColor accent READ accent WRITE setAccent NOTIFY accentChanged)
    Q_PROPERTY(bool autoAccent READ autoAccent WRITE setAutoAccent NOTIFY autoAccentChanged)
    Q_PROPERTY(int workspaces READ workspaces WRITE setWorkspaces NOTIFY workspacesChanged)
    Q_PROPERTY(bool dnd READ dnd WRITE setDnd NOTIFY dndChanged)

public:
    static LoaliSettings *create(QQmlEngine *engine, QJSEngine *jsEngine);

    QString wallpaper() const { return m_wallpaper; }
    void setWallpaper(const QString &value);

    qreal dim() const { return m_dim; }
    void setDim(qreal value);

    QColor accent() const { return m_accent; }
    void setAccent(const QColor &value);

    bool autoAccent() const { return m_autoAccent; }
    void setAutoAccent(bool value);

    int workspaces() const { return m_workspaces; }
    void setWorkspaces(int value);

    bool dnd() const { return m_dnd; }
    void setDnd(bool value);

signals:
    void wallpaperChanged();
    void dimChanged();
    void accentChanged();
    void autoAccentChanged();
    void workspacesChanged();
    void dndChanged();

private:
    explicit LoaliSettings(QObject *parent = nullptr);

    static LoaliSettings *s_instance;
    QString m_wallpaper;
    qreal m_dim;
    QColor m_accent;
    bool m_autoAccent = true;
    int m_workspaces;
    bool m_dnd = false;
};
