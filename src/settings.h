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
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(int taskbarStyle READ taskbarStyle WRITE setTaskbarStyle NOTIFY taskbarStyleChanged)
    Q_PROPERTY(int dockMode READ dockMode WRITE setDockMode NOTIFY dockModeChanged)
    Q_PROPERTY(qreal panelScale READ panelScale WRITE setPanelScale NOTIFY panelScaleChanged)

    enum BarStyle
    {
        Floating,
        Full
    };
    Q_ENUM(BarStyle)
    enum DockLook
    {
        Island,
        Plane
    };
    Q_ENUM(DockLook)

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

    bool dark() const { return m_dark; }
    void setDark(bool value);

    int taskbarStyle() const { return m_taskbarStyle; }
    void setTaskbarStyle(int value);

    int dockMode() const { return m_dockMode; }
    void setDockMode(int value);

    qreal panelScale() const { return m_panelScale; }
    void setPanelScale(qreal value);

signals:
    void wallpaperChanged();
    void dimChanged();
    void accentChanged();
    void autoAccentChanged();
    void workspacesChanged();
    void dndChanged();
    void darkChanged();
    void taskbarStyleChanged();
    void dockModeChanged();
    void panelScaleChanged();

private:
    explicit LoaliSettings(QObject *parent = nullptr);

    static LoaliSettings *s_instance;
    QString m_wallpaper;
    qreal m_dim;
    QColor m_accent;
    bool m_autoAccent = true;
    int m_workspaces;
    bool m_dnd = false;
    bool m_dark = true;
    int m_taskbarStyle = Floating;
    int m_dockMode = Island;
    qreal m_panelScale = 0.9;
};
