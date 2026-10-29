#include "matcolor.h"

#include <QBuffer>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QQmlEngine>
#include <QUrl>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <map>
#include <vector>

#include "cpp/cam/hct.h"
#include "cpp/dynamiccolor/material_dynamic_colors.h"
#include "cpp/quantize/wu.h"
#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_fruit_salad.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_rainbow.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"
#include "cpp/score/score.h"
#include "cpp/utils/utils.h"

MatColor *MatColor::s_instance = nullptr;

namespace {

QColor toColor(uint32_t argb)
{
    return QColor(static_cast<int>((argb >> 16) & 0xff),
                  static_cast<int>((argb >> 8) & 0xff),
                  static_cast<int>(argb & 0xff),
                  static_cast<int>((argb >> 24) & 0xff));
}

uint32_t toArgb(const QColor &color)
{
    return (static_cast<uint32_t>(color.alpha()) << 24)
         | (static_cast<uint32_t>(color.red()) << 16)
         | (static_cast<uint32_t>(color.green()) << 8)
         | static_cast<uint32_t>(color.blue());
}

QVariantMap schemeRoles(const material_color_utilities::DynamicScheme &s)
{
    using material_color_utilities::MaterialDynamicColors;
    QVariantMap m;
    m.insert(QStringLiteral("primary"), toColor(MaterialDynamicColors::Primary().GetArgb(s)));
    m.insert(QStringLiteral("onPrimary"), toColor(MaterialDynamicColors::OnPrimary().GetArgb(s)));
    m.insert(QStringLiteral("primaryContainer"), toColor(MaterialDynamicColors::PrimaryContainer().GetArgb(s)));
    m.insert(QStringLiteral("onPrimaryContainer"), toColor(MaterialDynamicColors::OnPrimaryContainer().GetArgb(s)));
    m.insert(QStringLiteral("secondary"), toColor(MaterialDynamicColors::Secondary().GetArgb(s)));
    m.insert(QStringLiteral("onSecondary"), toColor(MaterialDynamicColors::OnSecondary().GetArgb(s)));
    m.insert(QStringLiteral("secondaryContainer"), toColor(MaterialDynamicColors::SecondaryContainer().GetArgb(s)));
    m.insert(QStringLiteral("onSecondaryContainer"), toColor(MaterialDynamicColors::OnSecondaryContainer().GetArgb(s)));
    m.insert(QStringLiteral("tertiary"), toColor(MaterialDynamicColors::Tertiary().GetArgb(s)));
    m.insert(QStringLiteral("onTertiary"), toColor(MaterialDynamicColors::OnTertiary().GetArgb(s)));
    m.insert(QStringLiteral("tertiaryContainer"), toColor(MaterialDynamicColors::TertiaryContainer().GetArgb(s)));
    m.insert(QStringLiteral("onTertiaryContainer"), toColor(MaterialDynamicColors::OnTertiaryContainer().GetArgb(s)));
    m.insert(QStringLiteral("error"), toColor(MaterialDynamicColors::Error().GetArgb(s)));
    m.insert(QStringLiteral("onError"), toColor(MaterialDynamicColors::OnError().GetArgb(s)));
    m.insert(QStringLiteral("errorContainer"), toColor(MaterialDynamicColors::ErrorContainer().GetArgb(s)));
    m.insert(QStringLiteral("onErrorContainer"), toColor(MaterialDynamicColors::OnErrorContainer().GetArgb(s)));
    m.insert(QStringLiteral("background"), toColor(MaterialDynamicColors::Background().GetArgb(s)));
    m.insert(QStringLiteral("onBackground"), toColor(MaterialDynamicColors::OnBackground().GetArgb(s)));
    m.insert(QStringLiteral("surface"), toColor(MaterialDynamicColors::Surface().GetArgb(s)));
    m.insert(QStringLiteral("onSurface"), toColor(MaterialDynamicColors::OnSurface().GetArgb(s)));
    m.insert(QStringLiteral("surfaceVariant"), toColor(MaterialDynamicColors::SurfaceVariant().GetArgb(s)));
    m.insert(QStringLiteral("onSurfaceVariant"), toColor(MaterialDynamicColors::OnSurfaceVariant().GetArgb(s)));
    m.insert(QStringLiteral("surfaceDim"), toColor(MaterialDynamicColors::SurfaceDim().GetArgb(s)));
    m.insert(QStringLiteral("surfaceBright"), toColor(MaterialDynamicColors::SurfaceBright().GetArgb(s)));
    m.insert(QStringLiteral("surfaceContainerLowest"), toColor(MaterialDynamicColors::SurfaceContainerLowest().GetArgb(s)));
    m.insert(QStringLiteral("surfaceContainerLow"), toColor(MaterialDynamicColors::SurfaceContainerLow().GetArgb(s)));
    m.insert(QStringLiteral("surfaceContainer"), toColor(MaterialDynamicColors::SurfaceContainer().GetArgb(s)));
    m.insert(QStringLiteral("surfaceContainerHigh"), toColor(MaterialDynamicColors::SurfaceContainerHigh().GetArgb(s)));
    m.insert(QStringLiteral("surfaceContainerHighest"), toColor(MaterialDynamicColors::SurfaceContainerHighest().GetArgb(s)));
    m.insert(QStringLiteral("inverseSurface"), toColor(MaterialDynamicColors::InverseSurface().GetArgb(s)));
    m.insert(QStringLiteral("inverseOnSurface"), toColor(MaterialDynamicColors::InverseOnSurface().GetArgb(s)));
    m.insert(QStringLiteral("inversePrimary"), toColor(MaterialDynamicColors::InversePrimary().GetArgb(s)));
    m.insert(QStringLiteral("outline"), toColor(MaterialDynamicColors::Outline().GetArgb(s)));
    m.insert(QStringLiteral("outlineVariant"), toColor(MaterialDynamicColors::OutlineVariant().GetArgb(s)));
    m.insert(QStringLiteral("shadow"), toColor(MaterialDynamicColors::Shadow().GetArgb(s)));
    m.insert(QStringLiteral("scrim"), toColor(MaterialDynamicColors::Scrim().GetArgb(s)));
    m.insert(QStringLiteral("surfaceTint"), toColor(MaterialDynamicColors::SurfaceTint().GetArgb(s)));
    return m;
}

}

MatColor::MatColor(QObject *parent)
    : QObject(parent)
{
}

MatColor *MatColor::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)
    if (!s_instance)
        s_instance = new MatColor();
    engine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void MatColor::generateFromImage(const QString &source)
{
    static const QHash<QString, QColor> seeds = {
        { QStringLiteral("bloom"), QColor(QStringLiteral("#5A4A9F")) },
        { QStringLiteral("dunes"), QColor(QStringLiteral("#B36A3C")) },
        { QStringLiteral("meadow"), QColor(QStringLiteral("#3E7A4A")) }
    };
    if (seeds.contains(source)) {
        m_suggested.clear();
        apply(toArgb(seeds.value(source)));
        return;
    }

    QString path = source;
    if (path.startsWith(QStringLiteral("file:")))
        path = QUrl(path).toLocalFile();
    else if (path.startsWith(QStringLiteral("qrc:")))
        path = QStringLiteral(":") + path.mid(4);
    else if (!QFile::exists(path) && !path.contains(QLatin1Char('/')))
        path = QStringLiteral(":/Loali/assets/wallpapers/") + path + QStringLiteral(".jpg");

    QImage img(path);
    if (img.isNull())
        return;
    img = img.convertToFormat(QImage::Format_ARGB32)
              .scaled(96, 96, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    using material_color_utilities::Argb;
    std::vector<Argb> pixels;
    pixels.reserve(static_cast<size_t>(img.width()) * img.height());
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x)
            pixels.push_back(static_cast<Argb>(line[x]));
    }

    std::vector<Argb> clusters = material_color_utilities::QuantizeWu(pixels, 64);
    if (clusters.empty())
        return;

    std::map<Argb, uint32_t> population;
    for (Argb px : pixels) {
        Argb best = clusters.front();
        int bestDist = INT_MAX;
        for (Argb q : clusters) {
            const int dr = static_cast<int>((px >> 16) & 0xff) - static_cast<int>((q >> 16) & 0xff);
            const int dg = static_cast<int>((px >> 8) & 0xff) - static_cast<int>((q >> 8) & 0xff);
            const int db = static_cast<int>(px & 0xff) - static_cast<int>(q & 0xff);
            const int d = dr * dr + dg * dg + db * db;
            if (d < bestDist) {
                bestDist = d;
                best = q;
            }
        }
        ++population[best];
    }

    material_color_utilities::ScoreOptions options;
    options.desired = 4;
    const std::vector<Argb> ranked = material_color_utilities::RankedSuggestions(population, options);
    if (ranked.empty())
        return;

    QVariantList suggested;
    for (Argb c : ranked)
        suggested.append(toColor(c).name(QColor::HexArgb));
    m_suggested = suggested;

    apply(ranked.front());
}

void MatColor::generateFromColor(const QColor &color)
{
    if (!color.isValid())
        return;
    m_suggested.clear();
    apply(toArgb(color));
}

void MatColor::setStyle(int style)
{
    if (style == m_style)
        return;
    m_style = style;
    rebuild();
}

void MatColor::setContrast(qreal contrast)
{
    if (qFuzzyCompare(m_contrast, contrast))
        return;
    m_contrast = contrast;
    rebuild();
}

void MatColor::apply(uint32_t argb)
{
    m_argb = argb;
    rebuild();
}

void MatColor::rebuild()
{
    using namespace material_color_utilities;
    const Hct hct(m_argb);
    switch (m_style) {
    case 1: {
        SchemeVibrant light(hct, false, m_contrast);
        SchemeVibrant dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    case 2: {
        SchemeExpressive light(hct, false, m_contrast);
        SchemeExpressive dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    case 3: {
        SchemeContent light(hct, false, m_contrast);
        SchemeContent dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    case 4: {
        SchemeFruitSalad light(hct, false, m_contrast);
        SchemeFruitSalad dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    case 5: {
        SchemeRainbow light(hct, false, m_contrast);
        SchemeRainbow dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    case 6: {
        SchemeMonochrome light(hct, false, m_contrast);
        SchemeMonochrome dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    default: {
        SchemeTonalSpot light(hct, false, m_contrast);
        SchemeTonalSpot dark(hct, true, m_contrast);
        m_light = schemeRoles(light);
        m_dark = schemeRoles(dark);
        break;
    }
    }
    m_source = toColor(m_argb).name(QColor::HexArgb);
    emit paletteChanged();
}
