#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml>

#include "apps.h"
#include "dbusmenu.h"
#include "iconprovider.h"
#include "matcolor.h"
#include "screencopy.h"
#include "settings.h"
#include "tray.h"
#include "xdgoutput.h"

int main(int argc, char *argv[])
{
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
    parser.addOption(fullscreenOption);
    parser.addOption(demoOption);
    parser.addOption(demoDelayOption);
    parser.process(app);

    qmlRegisterSingletonType<Apps>("Loali", 1, 0, "Apps",
                                   [](QQmlEngine *e, QJSEngine *j) -> QObject * { return Apps::create(e, j); });
    qmlRegisterSingletonType<LoaliSettings>("Loali", 1, 0, "Settings",
                                            [](QQmlEngine *e, QJSEngine *j) -> QObject * { return LoaliSettings::create(e, j); });
    qmlRegisterSingletonType<Tray>("Loali", 1, 0, "Tray",
                                   [](QQmlEngine *e, QJSEngine *j) -> QObject * { return Tray::create(e, j); });
    qmlRegisterSingletonType<MatColor>("Loali", 1, 0, "MatColor",
                                       [](QQmlEngine *e, QJSEngine *j) -> QObject * { return MatColor::create(e, j); });
    qmlRegisterType<TrayMenu>("Loali", 1, 0, "TrayMenu");
    qmlRegisterType<ScreencopyManagerQuickExtension>("Loali", 1, 0, "ScreencopyManager");
    qmlRegisterType<XdgOutputManagerQuickExtension>("Loali", 1, 0, "XdgOutputManager");

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("loali"), new IconProvider());
    engine.addImageProvider(QStringLiteral("sni"), new SniImageProvider());
    engine.rootContext()->setContextProperty(QStringLiteral("fullscreenMode"),
                                             parser.isSet(fullscreenOption));
    engine.load(QUrl(QStringLiteral("qrc:/Loali/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
