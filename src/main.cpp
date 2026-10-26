#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "iconprovider.h"
#include "tray.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Loali"));
    QGuiApplication::setOrganizationName(QStringLiteral("Loali"));
    QGuiApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption fullscreenOption(QStringLiteral("fullscreen"),
                                        QStringLiteral("Run the output window in fullscreen mode."));
    parser.addOption(fullscreenOption);
    parser.process(app);

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
