#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("LoaliDemo"));

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
    QCommandLineOption appOpt(QStringList{QStringLiteral("a"), QStringLiteral("app")},
                              QStringLiteral("App look: terminal|files|generic"), QStringLiteral("app"),
                              QStringLiteral("generic"));
    parser.addOption(titleOpt);
    parser.addOption(colorOpt);
    parser.addOption(msgOpt);
    parser.addOption(appOpt);
    parser.process(app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("demoTitle"), parser.value(titleOpt));
    engine.rootContext()->setContextProperty(QStringLiteral("demoColor"), parser.value(colorOpt));
    engine.rootContext()->setContextProperty(QStringLiteral("demoMessage"), parser.value(msgOpt));
    engine.rootContext()->setContextProperty(QStringLiteral("demoApp"), parser.value(appOpt));
    engine.loadData(QByteArrayLiteral(R"(
import QtQuick
import QtQuick.Window

Window {
    id: win
    width: 660
    height: 440
    visible: true
    title: demoTitle
    color: "transparent"

    readonly property bool isTerm: demoApp === "terminal"
    readonly property bool isFiles: demoApp === "files"

    Rectangle {
        id: chrome
        anchors.fill: parent
        radius: 12
        color: win.isTerm ? "#15151E" : win.isFiles ? "#1C1B22" : demoColor
        clip: true

        Rectangle {
            visible: !win.isTerm
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.lighter(demoColor, 1.3) }
                GradientStop { position: 1; color: Qt.darker(demoColor, 1.35) }
            }
        }

        Rectangle {
            id: topbar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 40
            color: Qt.rgba(0, 0, 0, 0.28)

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Rectangle {
                    width: 22
                    height: 22
                    radius: 6
                    anchors.verticalCenter: parent.verticalCenter
                    color: demoColor
                    Text {
                        anchors.centerIn: parent
                        text: win.isFiles ? "F" : ">_"
                        color: "#FFFFFF"
                        font.pixelSize: win.isFiles ? 13 : 10
                        font.bold: true
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: win.isFiles ? "/home/loali" : "loali@loali: ~"
                    color: "#E8E6EE"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Repeater {
                    model: [
                        { c: "#5F6368", s: 8 },
                        { c: "#5F6368", s: 8 },
                        { c: "#EA4337", s: 10 }
                    ]

                    Rectangle {
                        required property var modelData
                        width: modelData.s + 8
                        height: modelData.s + 8
                        radius: (modelData.s + 8) / 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: "transparent"
                        Rectangle {
                            anchors.centerIn: parent
                            width: modelData.s
                            height: modelData.s
                            radius: modelData.s / 2
                            color: parent.parent.modelData.c
                        }
                    }
                }
            }
        }

        Rectangle {
            id: sidebar
            visible: win.isFiles
            anchors.top: topbar.bottom
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: 170
            color: Qt.rgba(0, 0, 0, 0.18)

            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 2

                Repeater {
                    model: [
                        { n: "Home", i: "HM" },
                        { n: "Documents", i: "DC" },
                        { n: "Downloads", i: "DL" },
                        { n: "Pictures", i: "PC" },
                        { n: "Music", i: "MS" },
                        { n: "Trash", i: "TR" }
                    ]

                    Rectangle {
                        required property var modelData
                        width: parent.width
                        height: 34
                        radius: 8
                        color: modelData.n === "Home" ? Qt.rgba(demoColor.r, demoColor.g, demoColor.b, 0.5) : "transparent"

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.i
                            font.pixelSize: 14
                            color: "#E8E6EE"
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 36
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.n
                            font.pixelSize: 13
                            color: modelData.n === "Home" ? "#FFFFFF" : "#C9C6D4"
                            font.weight: modelData.n === "Home" ? Font.DemiBold : Font.Normal
                        }
                    }
                }
            }
        }

        Grid {
            visible: win.isFiles
            anchors.top: topbar.bottom
            anchors.left: sidebar.right
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 14
            columns: 4
            rowSpacing: 14
            columnSpacing: 10

            Repeater {
                model: [
                    { n: "Projects", t: "folder" },
                    { n: "loali.png", t: "img" },
                    { n: "notes.txt", t: "txt" },
                    { n: "build.sh", t: "sh" },
                    { n: "wallpaper.jpg", t: "img" },
                    { n: "README.md", t: "txt" },
                    { n: "Music", t: "folder" },
                    { n: "report.pdf", t: "pdf" }
                ]

                Column {
                    required property var modelData
                    spacing: 6

                    Rectangle {
                        width: 72
                        height: 72
                        radius: 14
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: modelData.t === "folder" ? Qt.rgba(demoColor.r, demoColor.g, demoColor.b, 0.45)
                             : modelData.t === "img" ? "#3D5A80"
                             : modelData.t === "pdf" ? "#8C3A3A"
                             : modelData.t === "sh" ? "#3A6B4F"
                             : "#45444E"

                        Text {
                            anchors.centerIn: parent
                            text: modelData.t === "folder" ? "DIR"
                                  : modelData.t === "img" ? "IMG"
                                  : modelData.t === "pdf" ? "PDF"
                                  : modelData.t === "sh" ? "SH"
                                  : "TXT"
                            font.pixelSize: 17
                            font.bold: true
                        }
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.n
                        color: "#D8D5E2"
                        font.pixelSize: 11
                    }
                }
            }
        }

        Column {
            visible: win.isTerm
            anchors.top: topbar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 14
            spacing: 5

            Repeater {
                model: [
                    { p: "loali@loali:~$ ", c: "uname -a", o: true },
                    { p: "", c: "Loali 0.6.0 wayland x86_64 GNU/Linux", o: true },
                    { p: "loali@loali:~$ ", c: "echo $WAYLAND_DISPLAY", o: true },
                    { p: "", c: "wayland-0", o: true },
                    { p: "loali@loali:~$ ", c: "ls ~/Projects", o: true },
                    { p: "", c: "loali-compositor  m3e-shapes  docs", o: true },
                    { p: "loali@loali:~$ ", c: "", o: false }
                ]

                Row {
                    required property var modelData
                    spacing: 0

                    Text {
                        text: modelData.p
                        color: "#7BD88F"
                        font.family: "monospace"
                        font.pixelSize: 13
                    }

                    Text {
                        text: modelData.c
                        color: "#E0DEE8"
                        font.family: "monospace"
                        font.pixelSize: 13
                    }

                    Rectangle {
                        visible: !modelData.o
                        width: 8
                        height: 15
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#E0DEE8"

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { to: 0; duration: 530 }
                            NumberAnimation { to: 1; duration: 530 }
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: !win.isTerm && !win.isFiles
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
            visible: !win.isTerm && !win.isFiles
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 160
            text: demoTitle
            color: "#FFFFFF"
            font.pixelSize: 17
                            font.bold: true
            font.weight: Font.DemiBold
        }

        Text {
            visible: !win.isTerm && !win.isFiles
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 208
            text: demoMessage
            color: Qt.rgba(1, 1, 1, 0.85)
            font.pixelSize: 15
        }

        Rectangle {
            visible: !win.isTerm
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 36
            width: 320
            height: 44
            radius: 22
            color: Qt.rgba(1, 1, 1, 0.16)

            TextInput {
                id: demoField
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                verticalAlignment: TextInput.AlignVCenter
                color: "#FFFFFF"
                font.pixelSize: 15
                focus: true
                onTextChanged: console.info("loali-demo: field text:", JSON.stringify(text))
            }
        }
    }
}
)"));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
