#include "settings.h"

#include <QCryptographicHash>
#include <QQmlEngine>
#include <QRandomGenerator>
#include <QSettings>
#include <algorithm>

LoaliSettings *LoaliSettings::s_instance = nullptr;

static const int kPinIterations = 60000;
static const QLatin1String kPinTag("pbkdf2-sha256");

static void pinPads(const QByteArray &password, QByteArray &ipad, QByteArray &opad)
{
    QByteArray k = password;
    if (k.size() > 64)
        k = QCryptographicHash::hash(k, QCryptographicHash::Sha256);
    if (k.size() < 64)
        k += QByteArray(64 - k.size(), '\0');
    ipad = QByteArray(64, '\x36');
    opad = QByteArray(64, '\x5c');
    for (int i = 0; i < 64; ++i) {
        ipad[i] = char(ipad.at(i) ^ k.at(i));
        opad[i] = char(opad.at(i) ^ k.at(i));
    }
}

static QByteArray hmacSha256(const QByteArray &ipad, const QByteArray &opad, const QByteArray &message)
{
    QCryptographicHash h(QCryptographicHash::Sha256);
    h.addData(ipad);
    h.addData(message);
    const QByteArray inner = h.result();
    h.reset();
    h.addData(opad);
    h.addData(inner);
    return h.result();
}

static QByteArray pbkdf2Sha256(const QByteArray &password, const QByteArray &salt, int iterations, int length)
{
    QByteArray ipad;
    QByteArray opad;
    pinPads(password, ipad, opad);
    QByteArray derived;
    quint32 block = 1;
    while (derived.size() < length) {
        QByteArray be(4, '\0');
        be[0] = char((block >> 24) & 0xff);
        be[1] = char((block >> 16) & 0xff);
        be[2] = char((block >> 8) & 0xff);
        be[3] = char(block & 0xff);
        QByteArray u = hmacSha256(ipad, opad, salt + be);
        QByteArray acc = u;
        for (int i = 1; i < iterations; ++i) {
            u = hmacSha256(ipad, opad, u);
            for (int j = 0; j < u.size(); ++j)
                acc[j] = char(acc.at(j) ^ u.at(j));
        }
        derived += acc;
        ++block;
    }
    derived.truncate(length);
    return derived;
}

static QString makePinRecord(const QString &pin)
{
    QByteArray salt(16, '\0');
    for (int i = 0; i < salt.size(); ++i)
        salt[i] = char(QRandomGenerator::system()->generate() & 0xff);
    const QByteArray key = pbkdf2Sha256(pin.toUtf8(), salt, kPinIterations, 32);
    return QStringLiteral("%1$%2$%3$%4$%5")
        .arg(kPinTag)
        .arg(kPinIterations)
        .arg(QString::fromLatin1(salt.toBase64()))
        .arg(QString::fromLatin1(key.toBase64()))
        .arg(pin.size());
}

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
    m_colorStyle = s.value(QStringLiteral("appearance/colorStyle"), TonalSpot).toInt();
    if (m_colorStyle < TonalSpot || m_colorStyle > Monochrome)
        m_colorStyle = TonalSpot;
    m_contrast = s.value(QStringLiteral("appearance/contrast"), 0.0).toReal();
    m_contrast = std::clamp(m_contrast, -1.0, 1.0);
    m_pinRecord = QString();
    m_pinLength = 0;
    const QString stored = s.value(QStringLiteral("lock/pin"), QString()).toString();
    const QStringList parts = stored.split(QLatin1Char('$'));
    if (parts.size() == 5 && parts.first() == kPinTag) {
        m_pinRecord = stored;
        m_pinLength = parts.last().toInt();
    } else if (!stored.isEmpty()) {
        m_pinRecord = makePinRecord(stored);
        m_pinLength = stored.size();
        s.setValue(QStringLiteral("lock/pin"), m_pinRecord);
    }
}

LoaliSettings *LoaliSettings::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)
    if (!s_instance)
        s_instance = new LoaliSettings();
    if (engine)
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

void LoaliSettings::setColorStyle(int value)
{
    if (value < TonalSpot || value > Monochrome)
        value = TonalSpot;
    if (m_colorStyle == value)
        return;
    m_colorStyle = value;
    QSettings s;
    s.setValue(QStringLiteral("appearance/colorStyle"), value);
    emit colorStyleChanged();
}

void LoaliSettings::setContrast(qreal value)
{
    value = std::clamp(value, -1.0, 1.0);
    if (qFuzzyCompare(m_contrast, value))
        return;
    m_contrast = value;
    QSettings s;
    s.setValue(QStringLiteral("appearance/contrast"), value);
    emit contrastChanged();
}

bool LoaliSettings::verifyPin(const QString &attempt) const
{
    if (m_pinRecord.isEmpty() || attempt.size() != m_pinLength)
        return false;
    const QStringList parts = m_pinRecord.split(QLatin1Char('$'));
    if (parts.size() != 5 || parts.first() != kPinTag)
        return false;
    const QByteArray salt = QByteArray::fromBase64(parts.at(2).toLatin1());
    const QByteArray expect = QByteArray::fromBase64(parts.at(3).toLatin1());
    const QByteArray got = pbkdf2Sha256(attempt.toUtf8(), salt, parts.at(1).toInt(), expect.size());
    if (got.size() != expect.size())
        return false;
    char diff = 0;
    for (int i = 0; i < expect.size(); ++i)
        diff |= char(got.at(i) ^ expect.at(i));
    return diff == 0;
}

void LoaliSettings::setPin(const QString &value)
{
    QString v = value.trimmed();
    if (v.size() > 8)
        v.truncate(8);
    for (const QChar &c : v) {
        if (!c.isDigit())
            return;
    }
    if (v.isEmpty()) {
        if (m_pinRecord.isEmpty())
            return;
        m_pinRecord = QString();
        m_pinLength = 0;
    } else {
        m_pinRecord = makePinRecord(v);
        m_pinLength = v.size();
    }
    QSettings s;
    s.setValue(QStringLiteral("lock/pin"), m_pinRecord);
    emit pinChanged();
}
