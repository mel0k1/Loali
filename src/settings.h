#pragma once

#include <QColor>
#include <QObject>
#include <qqml.h>

class LoaliSettings : public QObject
{
    Q_OBJECT
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
    Q_PROPERTY(int colorStyle READ colorStyle WRITE setColorStyle NOTIFY colorStyleChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(bool pinSet READ pinSet NOTIFY pinChanged)
    Q_PROPERTY(int pinLength READ pinLength NOTIFY pinChanged)

public:
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
    enum ColorStyle
    {
        TonalSpot,
        Vibrant,
        Expressive,
        Content,
        FruitSalad,
        Rainbow,
        Monochrome
    };
    Q_ENUM(ColorStyle)

    static LoaliSettings *create(QQmlEngine *engine, QJSEngine *jsEngine);
    static LoaliSettings *instance() { return s_instance; }

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

    int colorStyle() const { return m_colorStyle; }
    void setColorStyle(int value);

    qreal contrast() const { return m_contrast; }
    void setContrast(qreal value);

    bool pinSet() const { return !m_pinRecord.isEmpty(); }
    int pinLength() const { return m_pinLength; }
    Q_INVOKABLE bool verifyPin(const QString &attempt) const;
    void setPin(const QString &value);

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
    void colorStyleChanged();
    void contrastChanged();
    void pinChanged();

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
    int m_colorStyle = TonalSpot;
    qreal m_contrast = 0.0;
    QString m_pinRecord;
    int m_pinLength = 0;
};
