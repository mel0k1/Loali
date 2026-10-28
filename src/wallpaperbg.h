#pragma once

#include <QColor>
#include <QHash>
#include <QImage>
#include <QQuickImageProvider>
#include <QRect>

class WallpaperBgProvider : public QQuickImageProvider
{
public:
    static WallpaperBgProvider *instance();

    WallpaperBgProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    static QColor averageColor(const QImage &image);

private:
    struct Params
    {
        QString source;
        QRect region;
        QSize canvas;
        int radius = 24;
        QColor tint;
        bool valid = false;
    };

    QImage baseCanvas(const QString &source, const QSize &canvas);
    QImage loadFile(const QString &source);
    QImage renderProcedural(const QString &style, const QSize &canvas);
    QImage sampleRegion(const QImage &base, const Params &p);
    void stackBlur(QImage &image, int radius);
    void applyTint(QImage &image, const QColor &tint);
    void roundCorners(QImage &image, int radius);

    static WallpaperBgProvider *s_instance;
    QHash<QString, QImage> m_fileCache;
    QHash<QString, QImage> m_resultCache;
};
