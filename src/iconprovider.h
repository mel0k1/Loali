#pragma once

#include <QImage>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>

class IconProvider : public QQuickImageProvider
{
public:
    explicit IconProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QImage fallback(const QString &name, int side) const;
};
