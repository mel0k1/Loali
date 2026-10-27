#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QQmlEngine>
#include <QString>
#include <QVariant>
#include <qqml.h>

class Apps;

class DesktopModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IconRole,
        AppIdRole,
        FolderIdRole,
        IsFolderRole
    };

    explicit DesktopModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Row {
        bool isFolder = false;
        QString folderId;
        QString appId;
        QString name;
        QString icon;
    };

    void rebuild();

    QList<Row> m_rows;
};
