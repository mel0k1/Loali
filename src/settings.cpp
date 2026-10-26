#include "settings.h"

#include <QQmlEngine>
#include <QSettings>
#include <algorithm>

LoaliSettings *LoaliSettings::s_instance = nullptr;

LoaliSettings::LoaliSettings(QObject *parent)
    : QObject(parent)
{
    QSettings s;
    m_wallpaper = s.value(QStringLiteral("wallpaper/style"), QStringLiteral("dusk")).toString();
    m_dim = s.value(QStringLiteral("wallpaper/dim"), 0.25).toReal();
    m_dim = std::clamp(m_dim, 0.0, 0.6);
    m_accent = s.value(QStringLiteral("appearance/accent"), QStringLiteral("#D0BCFF")).toString();
    m_autoAccent = s.value(QStringLiteral("appearance/autoAccent"), true).toBool();
    m_workspaces = s.value(QStringLiteral("shell/workspaces"), 4).toInt();
    m_workspaces = std::clamp(m_workspaces, 2, 6);
    m_dnd = s.value(QStringLiteral("notifications/dnd"), false).toBool();
    m_dark = s.value(QStringLiteral("appearance/dark"), true).toBool();
    m_taskbarStyle = s.value(QStringLiteral("panels/taskbarStyle"), Floating).toInt();
    if (m_taskbarStyle != Full)
        m_taskbarStyle = Floating;
    m_dockMode = s.value(QStringLiteral("panels/dockMode"), Island).toInt();
    if (m_dockMode != Plane)
        m_dockMode = Island;
    m_panelScale = s.value(QStringLiteral("panels/scale"), 0.9).toReal();
    m_panelScale = std::clamp(m_panelScale, 0.7, 1.3);
}

LoaliSettings *LoaliSettings::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)
    if (!s_instance)
        s_instance = new LoaliSettings();
    engine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void LoaliSettings::setWallpaper(const QString &value)
{
    if (m_wallpaper == value)
        return;
    m_wallpaper = value;
    QSettings s;
    s.setValue(QStringLiteral("wallpaper/style"), value);
    emit wallpaperChanged();
}

void LoaliSettings::setDim(qreal value)
{
    value = std::clamp(value, 0.0, 0.6);
    if (qFuzzyCompare(m_dim, value))
        return;
    m_dim = value;
    QSettings s;
    s.setValue(QStringLiteral("wallpaper/dim"), value);
    emit dimChanged();
}

void LoaliSettings::setAccent(const QColor &value)
{
    if (m_accent == value)
        return;
    m_accent = value;
    QSettings s;
    s.setValue(QStringLiteral("appearance/accent"), value.name());
    emit accentChanged();
}

void LoaliSettings::setAutoAccent(bool value)
{
    if (m_autoAccent == value)
        return;
    m_autoAccent = value;
    QSettings s;
    s.setValue(QStringLiteral("appearance/autoAccent"), value);
    emit autoAccentChanged();
}

void LoaliSettings::setDark(bool value)
{
    if (m_dark == value)
        return;
    m_dark = value;
    QSettings s;
    s.setValue(QStringLiteral("appearance/dark"), value);
    emit darkChanged();
}

void LoaliSettings::setWorkspaces(int value)
{
    value = std::clamp(value, 2, 6);
    if (m_workspaces == value)
        return;
    m_workspaces = value;
    QSettings s;
    s.setValue(QStringLiteral("shell/workspaces"), value);
    emit workspacesChanged();
}

void LoaliSettings::setDnd(bool value)
{
    if (m_dnd == value)
        return;
    m_dnd = value;
    QSettings s;
    s.setValue(QStringLiteral("notifications/dnd"), value);
    emit dndChanged();
}

void LoaliSettings::setTaskbarStyle(int value)
{
    if (value != Floating && value != Full)
        value = Floating;
    if (m_taskbarStyle == value)
        return;
    m_taskbarStyle = value;
    QSettings s;
    s.setValue(QStringLiteral("panels/taskbarStyle"), value);
    emit taskbarStyleChanged();
}

void LoaliSettings::setDockMode(int value)
{
    if (value != Island && value != Plane)
        value = Island;
    if (m_dockMode == value)
        return;
    m_dockMode = value;
    QSettings s;
    s.setValue(QStringLiteral("panels/dockMode"), value);
    emit dockModeChanged();
}

void LoaliSettings::setPanelScale(qreal value)
{
    value = std::clamp(value, 0.7, 1.3);
    if (qFuzzyCompare(m_panelScale, value))
        return;
    m_panelScale = value;
    QSettings s;
    s.setValue(QStringLiteral("panels/scale"), value);
    emit panelScaleChanged();
}
