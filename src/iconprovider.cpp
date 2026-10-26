#include "iconprovider.h"

#include <QFont>
#include <QIcon>
#include <QPainter>

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int side = requestedSize.width() > 0 ? requestedSize.width() : 64;
    const QString name = id.section(QLatin1Char('/'), 0, 0);
    QImage result;
    if (!name.isEmpty() && QIcon::hasThemeIcon(name)) {
        const QPixmap pm = QIcon::fromTheme(name).pixmap(side, side);
        if (!pm.isNull())
            result = pm.toImage();
    }
    if (result.isNull())
        result = fallback(name, side);
    if (size)
        *size = result.size();
    return result;
}

QImage IconProvider::fallback(const QString &name, int side) const
{
    QImage img(side, side, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x4F, 0x37, 0x8B));
    p.drawRoundedRect(QRectF(side * 0.06, side * 0.06, side * 0.88, side * 0.88), side * 0.24, side * 0.24);
    p.setPen(QColor(0xEA, 0xDD, 0xFF));
    QFont f = p.font();
    f.setPixelSize(qMax(10, side * 2 / 5));
    f.setBold(true);
    p.setFont(f);
    const QString letter = name.isEmpty() ? QStringLiteral("?") : name.left(1).toUpper();
    p.drawText(img.rect(), Qt::AlignCenter, letter);
    return img;
}
