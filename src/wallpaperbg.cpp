#include "wallpaperbg.h"

#include <QDir>
#include <QFile>
#include <QPainter>
#include <QRadialGradient>
#include <QUrl>
#include <algorithm>
#include <cmath>

WallpaperBgProvider *WallpaperBgProvider::s_instance = nullptr;

WallpaperBgProvider *WallpaperBgProvider::instance()
{
    if (!s_instance)
        s_instance = new WallpaperBgProvider();
    return s_instance;
}

WallpaperBgProvider::WallpaperBgProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

namespace {

struct PaletteDef
{
    const char *top;
    const char *bottom;
    int blobCount;
    struct
    {
        float x, y, r;
        const char *color;
        float alpha;
    } blobs[3];
};

static const PaletteDef s_palettes[] = {
    { "#17131F", "#0F0E14", 3,
      { { 0.85f, 0.18f, 0.55f, "#5A4A9F", 0.42f },
        { 0.08f, 0.82f, 0.60f, "#7A4A5E", 0.38f },
        { 0.50f, 0.60f, 0.45f, "#3E4A7A", 0.30f } } },
    { "#1F1410", "#140E0C", 3,
      { { 0.20f, 0.22f, 0.60f, "#B36A3C", 0.40f },
        { 0.80f, 0.75f, 0.55f, "#8A4A5E", 0.38f },
        { 0.50f, 0.45f, 0.42f, "#C98A4A", 0.28f } } },
    { "#101710", "#0C120C", 3,
      { { 0.80f, 0.75f, 0.60f, "#3E7A4A", 0.40f },
        { 0.15f, 0.20f, 0.50f, "#4A7A6E", 0.35f },
        { 0.50f, 0.85f, 0.45f, "#6E9A3E", 0.30f } } },
};

static const char *const s_paletteNames[] = { "bloom", "dunes", "meadow" };

}

QImage WallpaperBgProvider::loadFile(const QString &source)
{
    QString path = source;
    if (path.startsWith(QStringLiteral("file:")))
        path = QUrl(path).toLocalFile();
    const QString key = path;
    const auto it = m_fileCache.constFind(key);
    if (it != m_fileCache.constEnd())
        return *it;
    QImage image(path);
    if (image.isNull() && QFile::exists(path))
        image.load(path);
    m_fileCache.insert(key, image);
    if (m_fileCache.size() > 8)
        m_fileCache.erase(m_fileCache.begin());
    return image;
}

QImage WallpaperBgProvider::renderProcedural(const QString &style, const QSize &canvas)
{
    const PaletteDef *pal = &s_palettes[0];
    for (int i = 0; i < 3; ++i) {
        if (style == QLatin1String(s_paletteNames[i]))
            pal = &s_palettes[i];
    }
    QImage image(canvas, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(pal->bottom));
    QLinearGradient gradient(0, 0, 0, canvas.height());
    gradient.setColorAt(0.0, QColor(pal->top));
    gradient.setColorAt(1.0, QColor(pal->bottom));
    QPainter painter(&image);
    painter.fillRect(0, 0, canvas.width(), canvas.height(), gradient);
    const float radiusBase = std::max(canvas.width(), canvas.height());
    painter.setRenderHint(QPainter::Antialiasing);
    for (int i = 0; i < pal->blobCount; ++i) {
        const auto &blob = pal->blobs[i];
        QRadialGradient radial(QPointF(blob.x * canvas.width(), blob.y * canvas.height()),
                               blob.r * radiusBase);
        radial.setColorAt(0.0, QColor(blob.color));
        radial.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.setOpacity(blob.alpha);
        painter.fillRect(0, 0, canvas.width(), canvas.height(), radial);
        painter.setOpacity(1.0);
    }
    painter.end();
    return image;
}

QImage WallpaperBgProvider::baseCanvas(const QString &source, const QSize &canvas)
{
    if (canvas.isEmpty())
        return QImage();
    if (source == QStringLiteral("dusk"))
        return loadFile(QStringLiteral(":/Loali/assets/wallpapers/dusk.jpg"))
            .scaled(canvas, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if (source.startsWith(QStringLiteral("file:"))) {
        const QImage file = loadFile(source);
        if (file.isNull())
            return renderProcedural(QStringLiteral("bloom"), canvas);
        return file.scaled(canvas, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    return renderProcedural(source, canvas);
}

QImage WallpaperBgProvider::sampleRegion(const QImage &base, const Params &p)
{
    if (base.isNull() || p.region.isEmpty())
        return QImage();
    const QRect clipped = p.region.intersected(base.rect());
    if (clipped.isEmpty())
        return QImage();
    QImage sample = base.copy(clipped);
    const int workWidth = std::max(8, sample.width() / 4);
    const int workHeight = std::max(8, sample.height() / 4);
    sample = sample.scaled(workWidth, workHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    stackBlur(sample, 12);
    sample = sample.scaled(clipped.width(), clipped.height(), Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
    return sample;
}

void WallpaperBgProvider::stackBlur(QImage &image, int radius)
{
    if (radius < 1 || image.isNull())
        return;
    const int width = image.width();
    const int height = image.height();
    QRgb *pixels = reinterpret_cast<QRgb *>(image.bits());
    const int stride = image.bytesPerLine() / 4;
    QVector<QRgb> buffer(std::max(width, height));
    const int div = 2 * radius + 1;
    for (int y = 0; y < height; ++y) {
        QRgb *row = pixels + y * stride;
        int sr = 0;
        int sg = 0;
        int sb = 0;
        int sa = 0;
        for (int i = -radius; i <= radius; ++i) {
            const QRgb px = row[std::clamp(i, 0, width - 1)];
            sr += qRed(px);
            sg += qGreen(px);
            sb += qBlue(px);
            sa += qAlpha(px);
        }
        for (int x = 0; x < width; ++x) {
            buffer[x] = qRgba(sr / div, sg / div, sb / div, sa / div);
            const int outIdx = std::clamp(x - radius, 0, width - 1);
            const int inIdx = std::clamp(x + radius + 1, 0, width - 1);
            sr += qRed(row[inIdx]) - qRed(row[outIdx]);
            sg += qGreen(row[inIdx]) - qGreen(row[outIdx]);
            sb += qBlue(row[inIdx]) - qBlue(row[outIdx]);
            sa += qAlpha(row[inIdx]) - qAlpha(row[outIdx]);
        }
        memcpy(row, buffer.constData(), width * sizeof(QRgb));
    }
    for (int x = 0; x < width; ++x) {
        int sr = 0;
        int sg = 0;
        int sb = 0;
        int sa = 0;
        for (int i = -radius; i <= radius; ++i) {
            const QRgb px = pixels[std::clamp(i, 0, height - 1) * stride + x];
            sr += qRed(px);
            sg += qGreen(px);
            sb += qBlue(px);
            sa += qAlpha(px);
        }
        for (int y = 0; y < height; ++y) {
            buffer[y] = qRgba(sr / div, sg / div, sb / div, sa / div);
            const int outIdx = std::clamp(y - radius, 0, height - 1);
            const int inIdx = std::clamp(y + radius + 1, 0, height - 1);
            sr += qRed(pixels[inIdx * stride + x]) - qRed(pixels[outIdx * stride + x]);
            sg += qGreen(pixels[inIdx * stride + x]) - qGreen(pixels[outIdx * stride + x]);
            sb += qBlue(pixels[inIdx * stride + x]) - qBlue(pixels[outIdx * stride + x]);
            sa += qAlpha(pixels[inIdx * stride + x]) - qAlpha(pixels[outIdx * stride + x]);
        }
        for (int y = 0; y < height; ++y)
            pixels[y * stride + x] = buffer[y];
    }
}

void WallpaperBgProvider::applyTint(QImage &image, const QColor &tint)
{
    if (!tint.isValid() || tint.alpha() == 0)
        return;
    const qreal alpha = tint.alphaF();
    const int tr = tint.red();
    const int tg = tint.green();
    const int tb = tint.blue();
    const qreal tl = 0.2126 * tint.redF() + 0.7152 * tint.greenF() + 0.0722 * tint.blueF();
    for (int y = 0; y < image.height(); ++y) {
        QRgb *row = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb px = row[x];
            qreal r = qRed(px) * (1 - alpha) + tr * alpha;
            qreal g = qGreen(px) * (1 - alpha) + tg * alpha;
            qreal b = qBlue(px) * (1 - alpha) + tb * alpha;
            qreal lum = (0.2126 * r + 0.7152 * g + 0.0722 * b) / 255.0;
            if (lum > 0.001) {
                qreal target = tl + (lum - tl) * 0.5;
                target = qBound(tl - 0.2, target, tl + 0.2);
                const qreal k = target / lum;
                r *= k;
                g *= k;
                b *= k;
                const qreal outLum = (0.2126 * r + 0.7152 * g + 0.0722 * b) / 255.0;
                r = outLum * 255.0 + (r - outLum * 255.0) * 1.45;
                g = outLum * 255.0 + (g - outLum * 255.0) * 1.45;
                b = outLum * 255.0 + (b - outLum * 255.0) * 1.45;
            }
            row[x] = qRgba(int(qBound<qreal>(0.0, r, 255.0)), int(qBound<qreal>(0.0, g, 255.0)),
                           int(qBound<qreal>(0.0, b, 255.0)), qAlpha(px));
        }
    }
}

void WallpaperBgProvider::roundCorners(QImage &image, int radius)
{
    if (radius <= 0)
        return;
    const int width = image.width();
    const int height = image.height();
    const int r = std::min(radius, std::min(width, height) / 2);
    if (r < 1)
        return;
    for (int y = 0; y < height; ++y) {
        QRgb *row = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const int dx = x < r ? r - x : (x >= width - r ? x - (width - 1 - r) : 0);
            const int dy = y < r ? r - y : (y >= height - r ? y - (height - 1 - r) : 0);
            if (dx > 0 && dy > 0) {
                const int dist2 = dx * dx + dy * dy;
                if (dist2 > r * r)
                    row[x] = qRgba(0, 0, 0, 0);
            }
        }
    }
}

QImage WallpaperBgProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)
    const QString key = QUrl::fromPercentEncoding(id.toUtf8());
    const auto cached = m_resultCache.constFind(key);
    if (cached != m_resultCache.constEnd()) {
        if (size)
            *size = cached->size();
        return *cached;
    }
    Params p;
    const QStringList parts = key.split(QLatin1Char('|'));
    if (parts.size() >= 9) {
        p.source = parts.at(0);
        p.region = QRect(parts.at(1).toInt(), parts.at(2).toInt(), parts.at(3).toInt(),
                         parts.at(4).toInt());
        p.canvas = QSize(parts.at(5).toInt(), parts.at(6).toInt());
        p.radius = parts.at(7).toInt();
        p.tint = QColor(parts.at(8));
        if (parts.size() >= 10)
            p.tint.setAlpha(int(parts.at(9).toDouble() * 255));
        p.valid = true;
    }
    QImage result;
    if (p.valid) {
        const QImage base = baseCanvas(p.source, p.canvas);
        result = sampleRegion(base, p);
        applyTint(result, p.tint);
        roundCorners(result, p.radius);
    }
    m_resultCache.insert(key, result);
    if (m_resultCache.size() > 16)
        m_resultCache.erase(m_resultCache.begin());
    if (size)
        *size = result.size();
    return result;
}
