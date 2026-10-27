#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("LoaliDemo"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption titleOpt(QStringList{QStringLiteral("t"), QStringLiteral("title")},
                                QStringLiteral("Window title"), QStringLiteral("title"),
                                QStringLiteral("Loali demo"));
    QCommandLineOption colorOpt(QStringList{QStringLiteral("c"), QStringLiteral("color")},
                                QStringLiteral("Accent color"), QStringLiteral("color"),
                                QStringLiteral("#7C4DFF"));
    QCommandLineOption msgOpt(QStringList{QStringLiteral("m"), QStringLiteral("message")},
                              QStringLiteral("Body text"), QStringLiteral("message"),
                              QStringLiteral("Hello from Loali"));
    parser.addOption(titleOpt);
    parser.addOption(colorOpt);
    parser.addOption(msgOpt);
    parser.process(app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("demoTitle"), parser.value(titleOpt));
    engine.rootContext()->setContextProperty(QStringLiteral("demoColor"), parser.value(colorOpt));
    engine.rootContext()->setContextProperty(QStringLiteral("demoMessage"), parser.value(msgOpt));
    engine.loadData(QByteArrayLiteral(R"(
import QtQuick
import QtQuick.Window
Window {
    width: 620
    height: 420
    visible: true
    title: demoTitle
    color: demoColor
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.lighter(demoColor, 1.35) }
            GradientStop { position: 1; color: Qt.darker(demoColor, 1.25) }
        }
    }
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 48
        width: 84
        height: 84
        radius: 42
        color: Qt.rgba(1, 1, 1, 0.22)
        Rectangle {
            anchors.centerIn: parent
            width: 38
            height: 38
            radius: 19
            color: "#FFFFFF"
            opacity: 0.9
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 160
        text: demoTitle
        color: "#FFFFFF"
        font.pixelSize: 30
        font.weight: Font.DemiBold
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 208
        text: demoMessage
        color: Qt.rgba(1, 1, 1, 0.85)
        font.pixelSize: 15
    }
}
)"));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
