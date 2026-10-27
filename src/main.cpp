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
#include "notifications.h"
#include "screencopy.h"
#include "settings.h"
#include "tray.h"
#include "xdgoutput.h"

#include <QQuickWindow>
#include <QTimer>

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
