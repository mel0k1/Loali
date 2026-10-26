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
