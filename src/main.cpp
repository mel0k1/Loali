#include <QCommandLineParser>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QtQml>

#include "apps.h"
#include "dbusmenu.h"
#include "desktopmodel.h"
#include "iconprovider.h"
#include "layershell.h"
#include "matcolor.h"
#include "notifications.h"
#include "powerservice.h"
#include "screencopy.h"
#include "settings.h"
#include "tray.h"
#include "util.h"
#include "wallpaperbg.h"
#include "wloutput.h"
#include "xdgoutput.h"
#include "datacontrol.h"
#include "idle.h"
#include "inputmethod.h"
#include "blur.h"
#include "portals.h"
#include "sessionlock.h"
#include "sessionlockview.h"
#include "xwayland.h"

#include <QQuickWindow>
#include <QTimer>

namespace {

void registerIdleBridge()
{
    Idle::registerTypes();
    Portals::registerTypes();
}


void applyUiPalette()
{
    LoaliSettings *s = LoaliSettings::instance();
    if (!s)
        return;
    QGuiApplication *app = qobject_cast<QGuiApplication *>(QGuiApplication::instance());
    if (!app)
        return;
    QPalette pal = app->palette();
    const QColor windowText = s->dark() ? QColor(0xE6, 0xE0, 0xE9) : QColor(0x1D, 0x1B, 0x20);
    const QColor window = s->dark() ? QColor(0x14, 0x12, 0x18) : QColor(0xFE, 0xF7, 0xFF);
    const QColor surface = s->dark() ? QColor(0x2B, 0x29, 0x30) : QColor(0xF3, 0xED, 0xF7);
    const QColor tipBase = s->dark() ? QColor(0x36, 0x34, 0x3B) : QColor(0xE6, 0xE0, 0xE9);
    const QColor tipText = s->dark() ? QColor(0xE6, 0xE0, 0xE9) : QColor(0x1D, 0x1B, 0x20);
    const QColor placeholder = s->dark() ? QColor(0xCA, 0xC4, 0xD0) : QColor(0x49, 0x45, 0x4F);
    const QColor highlight = s->dark() ? QColor(0x4F, 0x37, 0x8B) : QColor(0x67, 0x50, 0xA4);
    const QColor highlightedText = s->dark() ? QColor(0xEA, 0xDD, 0xFF) : QColor(0xFF, 0xFF, 0xFF);
    for (QPalette::ColorGroup g : { QPalette::Active, QPalette::Inactive }) {
        pal.setColor(g, QPalette::Window, window);
        pal.setColor(g, QPalette::WindowText, windowText);
        pal.setColor(g, QPalette::Base, surface);
        pal.setColor(g, QPalette::AlternateBase, surface);
        pal.setColor(g, QPalette::Text, windowText);
        pal.setColor(g, QPalette::Button, surface);
        pal.setColor(g, QPalette::ButtonText, windowText);
        pal.setColor(g, QPalette::ToolTipBase, tipBase);
        pal.setColor(g, QPalette::ToolTipText, tipText);
        pal.setColor(g, QPalette::PlaceholderText, placeholder);
        pal.setColor(g, QPalette::Highlight, highlight);
        pal.setColor(g, QPalette::HighlightedText, highlightedText);
        pal.setColor(g, QPalette::BrightText, windowText);
        pal.setColor(g, QPalette::Link, highlight);
    }
    app->setPalette(pal);
}

void applyUiFont()
{
    const QStringList prefs = { QStringLiteral("Noto Sans"), QStringLiteral("Inter"),
                                QStringLiteral("Carlito"), QStringLiteral("DejaVu Sans") };
    const QStringList fams = QFontDatabase::families();
    for (const QString &f : prefs) {
        if (fams.contains(f)) {
            QGuiApplication::setFont(QFont(f));
            return;
        }
    }
}

}

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setSamples(qEnvironmentVariableIntValue("LOALI_MSAA") > 0 ? qEnvironmentVariableIntValue("LOALI_MSAA") : 8);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(fmt);
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Loali"));
    QGuiApplication::setOrganizationName(QStringLiteral("Loali"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.6.0"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption fullscreenOption(QStringLiteral("fullscreen"),
                                        QStringLiteral("Run the output window in fullscreen mode."));
    QCommandLineOption demoOption(QStringLiteral("demo"),
                                  QStringLiteral("Open <launcher|settings|expo> after start."),
                                  QStringLiteral("mode"));
    QCommandLineOption demoDelayOption(QStringLiteral("demo-delay"),
                                       QStringLiteral("Delay before the demo action, ms."),
                                       QStringLiteral("ms"));
    QCommandLineOption shotOption(QStringLiteral("screenshot"),
                                  QStringLiteral("Save a screenshot of the output to <file> and exit."),
                                  QStringLiteral("file"));
    QCommandLineOption shotDelayOption(QStringLiteral("screenshot-delay"),
                                       QStringLiteral("Delay before the screenshot, ms."),
                                       QStringLiteral("ms"),
                                       QStringLiteral("8000"));
    parser.addOption(fullscreenOption);
    parser.addOption(demoOption);
    parser.addOption(demoDelayOption);
    parser.addOption(shotOption);
    parser.addOption(shotDelayOption);
    parser.process(app);

    qmlRegisterSingletonType<Apps>("Loali", 1, 0, "Apps",
                                   [](QQmlEngine *e, QJSEngine *j) -> QObject * { return Apps::create(e, j); });
    qmlRegisterSingletonType<LoaliSettings>("Loali", 1, 0, "Settings",
                                            [](QQmlEngine *e, QJSEngine *j) -> QObject * { return LoaliSettings::create(e, j); });
    qmlRegisterSingletonType<Tray>("Loali", 1, 0, "Tray",
                                   [](QQmlEngine *e, QJSEngine *j) -> QObject * { return Tray::create(e, j); });
    qmlRegisterSingletonType<MatColor>("Loali", 1, 0, "MatColor",
                                       [](QQmlEngine *e, QJSEngine *j) -> QObject * { return MatColor::create(e, j); });
    qmlRegisterSingletonType<Notifications>("Loali", 1, 0, "Notifications",
                                            [](QQmlEngine *e, QJSEngine *j) -> QObject * { return Notifications::create(e, j); });
    registerIdleBridge();
    PowerService::registerType();
    qmlRegisterSingletonType<Idle>("Loali", 1, 0, "Idle",
                                   [](QQmlEngine *e, QJSEngine *j) -> QObject * { return Idle::create(e, j); });
    qmlRegisterSingletonType<LoaliUtil>("Loali", 1, 0, "Util",
                                        [](QQmlEngine *e, QJSEngine *j) -> QObject * { return LoaliUtil::create(e, j); });
    qmlRegisterType<TrayMenu>("Loali", 1, 0, "TrayMenu");
    qmlRegisterType<ScreencopyManagerQuickExtension>("Loali", 1, 0, "ScreencopyManager");
    qmlRegisterType<XdgOutputManagerQuickExtension>("Loali", 1, 0, "XdgOutputManager");
    qmlRegisterType<DesktopModel>("Loali", 1, 0, "DesktopModel");
    qmlRegisterType<WlOutputFix>("Loali", 1, 0, "WlOutputFix");
    qmlRegisterType<LayerShellQuickExtension>("Loali", 1, 0, "LayerShell");
    qmlRegisterType<XWaylandShellQuickExtension>("Loali", 1, 0, "XWaylandShell");
    qmlRegisterType<IdleManagerQuickExtension>("Loali", 1, 0, "IdleManager");
    qmlRegisterType<OutputPowerManagerQuickExtension>("Loali", 1, 0, "OutputPowerManager");
    qmlRegisterType<DataControlManagerQuickExtension>("Loali", 1, 0, "DataControlManager");
    qmlRegisterType<SessionLockManagerQuickExtension>("Loali", 1, 0, "SessionLockManager");
    qmlRegisterType<InputMethodManagerQuickExtension>("Loali", 1, 0, "InputMethodManager");
    qmlRegisterType<VirtualKeyboardManagerQuickExtension>("Loali", 1, 0, "VirtualKeyboardManager");
    qmlRegisterSingletonType<BlurRegistry>("Loali", 1, 0, "LoaliBlur",
                                           [](QQmlEngine *e, QJSEngine *j) -> QObject * { return BlurRegistry::create(e, j); });
    qmlRegisterType<KdeBlurManagerQuickExtension>("Loali", 1, 0, "KdeBlurManager");
    qmlRegisterType<ExtBackgroundEffectManagerQuickExtension>("Loali", 1, 0, "ExtBackgroundEffectManager");
    qmlRegisterUncreatableType<SessionLockSurface>("Loali", 1, 0, "SessionLockSurface",
                                                   QStringLiteral("created by the compositor"));
    qmlRegisterType<SessionLockView>("Loali", 1, 0, "SessionLockView");
    qmlRegisterUncreatableType<XWaylandSurface>("Loali", 1, 0, "XWaylandSurface",
                                                QStringLiteral("created by the compositor"));
    qmlRegisterType<LayerSurface>("Loali", 1, 0, "LayerSurface");

    applyUiFont();
    Apps::create(nullptr, nullptr);
    LoaliSettings::create(nullptr, nullptr);
    applyUiPalette();
    if (LoaliSettings::instance())
        QObject::connect(LoaliSettings::instance(), &LoaliSettings::darkChanged,
                         LoaliSettings::instance(), &applyUiPalette);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("loali"), new IconProvider());
    engine.addImageProvider(QStringLiteral("sni"), new SniImageProvider());
    engine.addImageProvider(QStringLiteral("loalibg"), WallpaperBgProvider::instance());
    engine.rootContext()->setContextProperty(QStringLiteral("fullscreenMode"),
                                             parser.isSet(fullscreenOption));
    engine.load(QUrl(QStringLiteral("qrc:/Loali/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    Portals::create();
    PowerService::start();
    if (parser.isSet(shotOption)) {
        const QString file = parser.value(shotOption);
        const int delay = parser.value(shotDelayOption).toInt();
        QTimer::singleShot(delay, [file] {
            const QList<QWindow *> windows = QGuiApplication::topLevelWindows();
            for (QWindow *w : windows) {
                auto *quick = qobject_cast<QQuickWindow *>(w);
                if (!quick)
                    continue;
                quick->grabWindow().save(file);
                QCoreApplication::quit();
                return;
            }
            QCoreApplication::exit(2);
        });
    }
    return app.exec();
}
