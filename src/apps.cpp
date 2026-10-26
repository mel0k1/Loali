#include "apps.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QProcess>
#include <algorithm>

Apps *Apps::s_instance = nullptr;

namespace {

const QRegularExpression &fieldCode()
{
    static QRegularExpression re(QStringLiteral("%[a-zA-Z~]"));
    return re;
}

} // namespace

Apps::Apps(QObject *parent)
    : QAbstractListModel(parent)
{
    scan();
    loadPins();
}

Apps *Apps::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)
    if (!s_instance)
        s_instance = new Apps();
    engine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

int Apps::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant Apps::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();
    const Entry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole:
        return e.name;
    case IconRole:
        return e.icon;
    case ExecRole:
        return e.exec;
    case IdRole:
        return e.id;
    case CommentRole:
        return e.comment;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> Apps::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IconRole] = "icon";
    roles[ExecRole] = "exec";
    roles[IdRole] = "appId";
    roles[CommentRole] = "comment";
    return roles;
}

void Apps::scan()
{
    QHash<QString, bool> seen;
    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString &dir : dirs) {
        QDirIterator it(dir, {QStringLiteral("*.desktop")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFile file(it.filePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            Entry e;
            e.id = QFileInfo(it.filePath()).completeBaseName();
            if (seen.contains(e.id))
                continue;
            bool inEntry = false;
            bool ok = false;
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString line = stream.readLine().trimmed();
                if (line.startsWith(QLatin1Char('['))) {
                    if (inEntry)
                        break;
                    inEntry = (line == QStringLiteral("[Desktop Entry]"));
                    continue;
                }
                if (!inEntry || line.startsWith(QLatin1Char('#')) || !line.contains(QLatin1Char('=')))
                    continue;
                const int eq = line.indexOf(QLatin1Char('='));
                const QString key = line.left(eq);
                const QString value = line.mid(eq + 1);
                if (key.contains(QLatin1Char('[')))
                    continue;
                if (key == QLatin1String("Type"))
                    ok = (value == QLatin1String("Application"));
                else if (key == QLatin1String("Name"))
                    e.name = value;
                else if (key == QLatin1String("Icon"))
                    e.icon = value;
                else if (key == QLatin1String("Exec"))
                    e.exec = value;
                else if (key == QLatin1String("Comment"))
                    e.comment = value;
                else if (key == QLatin1String("NoDisplay"))
                    e.hidden = e.hidden || value == QLatin1String("true");
                else if (key == QLatin1String("Hidden"))
                    e.hidden = e.hidden || value == QLatin1String("true");
            }
            if (ok && !e.hidden && !e.name.isEmpty() && !e.exec.isEmpty()) {
                e.exec.remove(fieldCode());
                e.exec = e.exec.trimmed();
                if (!e.exec.isEmpty()) {
                    m_entries.append(e);
                    seen.insert(e.id, true);
                }
            }
        }
    }
    std::sort(m_entries.begin(), m_entries.end(), [](const Entry &a, const Entry &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
}

void Apps::loadPins()
{
    QSettings settings;
    m_pins = settings.value(QStringLiteral("dock/pins")).toStringList();
    if (!m_pins.isEmpty())
        return;
    const QStringList keywords = {QStringLiteral("file"),
                                  QStringLiteral("terminal"),
                                  QStringLiteral("firefox"),
                                  QStringLiteral("chrom")};
    for (const QString &kw : keywords) {
        for (const Entry &e : m_entries) {
            if (m_pins.size() >= 4)
                break;
            if ((e.id.contains(kw, Qt::CaseInsensitive) || e.name.contains(kw, Qt::CaseInsensitive))
                && !m_pins.contains(e.id))
                m_pins.append(e.id);
        }
    }
}

QVariantList Apps::pinnedApps() const
{
    QVariantList res;
    for (const QString &id : m_pins) {
        const Entry *e = find(id);
        if (!e)
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("appId"), e->id);
        m.insert(QStringLiteral("name"), e->name);
        m.insert(QStringLiteral("icon"), e->icon);
        m.insert(QStringLiteral("comment"), e->comment);
        res.append(m);
    }
    return res;
}

QVariantList Apps::filterApps(const QString &query) const
{
    QVariantList res;
    const QString q = query.trimmed();
    for (const Entry &e : m_entries) {
        if (!q.isEmpty()
            && !e.name.contains(q, Qt::CaseInsensitive)
            && !e.comment.contains(q, Qt::CaseInsensitive)
            && !e.id.contains(q, Qt::CaseInsensitive))
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("appId"), e.id);
        m.insert(QStringLiteral("name"), e.name);
        m.insert(QStringLiteral("icon"), e.icon);
        m.insert(QStringLiteral("comment"), e.comment);
        res.append(m);
    }
    return res;
}

void Apps::launch(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return;
    launchExec(m_entries.at(row).exec);
}

void Apps::launchById(const QString &id) const
{
    const Entry *e = find(id);
    if (e)
        launchExec(e->exec);
}

bool Apps::isPinned(const QString &id) const
{
    return m_pins.contains(id);
}

void Apps::togglePin(const QString &id)
{
    if (m_pins.contains(id))
        m_pins.removeAll(id);
    else
        m_pins.append(id);
    QSettings settings;
    settings.setValue(QStringLiteral("dock/pins"), m_pins);
    emit pinsChanged();
}

QString Apps::iconForAppId(const QString &appId) const
{
    const QString target = normalizeId(appId);
    if (target.isEmpty())
        return QString();
    for (const Entry &e : m_entries) {
        if (normalizeId(e.id) == target)
            return e.icon;
    }
    for (const Entry &e : m_entries) {
        const QString nid = normalizeId(e.id);
        if (nid.contains(target) || target.contains(nid))
            return e.icon;
    }
    return QString();
}

const Apps::Entry *Apps::find(const QString &id) const
{
    for (const Entry &e : m_entries) {
        if (e.id == id)
            return &e;
    }
    return nullptr;
}

void Apps::launchExec(const QString &exec) const
{
    QProcess::startDetached(QStringLiteral("sh"), {QStringLiteral("-c"), exec});
}

QString Apps::normalizeId(QString id)
{
    if (id.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive))
        id.chop(8);
    return id.toLower();
}
