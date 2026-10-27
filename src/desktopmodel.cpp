#include "desktopmodel.h"

#include "apps.h"

DesktopModel::DesktopModel(QObject *parent)
    : QAbstractListModel(parent)
{
    Apps *a = Apps::instance();
    if (!a)
        return;
    connect(a, &Apps::foldersChanged, this, &DesktopModel::rebuild);
    connect(a, &Apps::countChanged, this, &DesktopModel::rebuild);
    connect(a, &Apps::pinsChanged, this, &DesktopModel::rebuild);
    rebuild();
}

int DesktopModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant DesktopModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case NameRole:
        return r.name;
    case IconRole:
        return r.icon;
    case AppIdRole:
        return r.appId;
    case FolderIdRole:
        return r.folderId;
    case IsFolderRole:
        return r.isFolder;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DesktopModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IconRole] = "icon";
    roles[AppIdRole] = "appId";
    roles[FolderIdRole] = "folderId";
    roles[IsFolderRole] = "isFolder";
    return roles;
}

void DesktopModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    Apps *a = Apps::instance();
    if (a) {
        const QVariantList fs = a->folders();
        for (const QVariant &v : fs) {
            const QVariantMap m = v.toMap();
            Row r;
            r.isFolder = true;
            r.folderId = m.value(QStringLiteral("id")).toString();
            r.name = m.value(QStringLiteral("name")).toString();
            r.icon = m.value(QStringLiteral("icon")).toString();
            m_rows.append(r);
        }
        const QVariantList as = a->allApps();
        for (const QVariant &v : as) {
            const QVariantMap m = v.toMap();
            const QString id = m.value(QStringLiteral("appId")).toString();
            if (a->isAppInFolder(id))
                continue;
            Row r;
            r.appId = id;
            r.name = m.value(QStringLiteral("name")).toString();
            r.icon = m.value(QStringLiteral("icon")).toString();
            m_rows.append(r);
        }
    }
    endResetModel();
}
