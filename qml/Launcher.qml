import QtQuick
import QtQuick.Controls
import Loali

Item {
    id: launcher

    property bool open: false
    property var shell: null

    anchors.fill: parent
    opacity: open ? 1 : 0
    visible: opacity > 0
    enabled: open
    Behavior on opacity { NumberAnimation { duration: 180 } }

    function filtered() {
        return Apps.filterApps(field.text)
    }

    function runningFor(appId) {
        return shell ? shell.windowsForApp(appId).length > 0 : false
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: 0.5
        MouseArea {
            anchors.fill: parent
            onClicked: launcher.open = false
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(760, launcher.width * 0.9)
        height: Math.min(640, launcher.height * 0.88)
        radius: 32
        color: Theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(Theme.outlineVariant, 0.6)
        scale: launcher.open ? 1 : 0.92
        Behavior on scale {
            NumberAnimation {
                duration: 260
                easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
            }
        }

        Shadow {
            extent: 16
            radius: 32
        }

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            Rectangle {
                width: parent.width
                height: 52
                radius: 26
                color: Theme.surfaceContainerHighest

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 10

                    MIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        height: 20
                        path: Theme.iconSearch
                        color: Theme.onSurfaceVariant
                    }

                    TextField {
                        id: field
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 30
                        placeholderText: qsTr("Search apps")
                        color: Theme.onSurface
                        placeholderTextColor: Theme.onSurfaceVariant
                        font.pixelSize: 15
                        background: null
                        onAccepted: {
                            const list = launcher.filtered()
                            if (list.length > 0) {
                                Apps.launchById(list[0].appId)
                                launcher.open = false
                                field.text = ""
                            }
                        }
                    }
                }
            }

            GridView {
                id: appsGrid
                width: parent.width
                height: parent.height - 66
                clip: true
                readonly property int columns: Math.max(4, Math.floor(width / 120))
                cellWidth: Math.floor(width / columns)
                cellHeight: 120
                model: Apps.filterApps(field.text)
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: appCell
                    required property var modelData
                    width: appsGrid.cellWidth
                    height: appsGrid.cellHeight
                    readonly property bool running: launcher.runningFor(appCell.modelData.appId)
                    scale: press.pressed ? 0.94 : 1
                    Behavior on scale {
                        NumberAnimation {
                            duration: 160
                            easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 20
                        color: press.containsMouse ? Qt.alpha(Theme.onSurface, 0.08) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 18
                        width: 56
                        height: 56
                        source: "image://loali/" + appCell.modelData.icon
                        sourceSize: Qt.size(112, 112)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: appCell.running ? 20 : 16
                        width: parent.width - 16
                        text: appCell.modelData.name
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        color: appCell.running ? Theme.primary : Theme.onSurface
                        font.pixelSize: 12
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                    }

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 8
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.primary
                        opacity: appCell.running ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }

                    MouseArea {
                        id: press
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (appCell.running && launcher.shell) {
                                const wins = launcher.shell.windowsForApp(appCell.modelData.appId)
                                if (wins.length > 0) {
                                    launcher.shell.activate(wins[0])
                                    launcher.open = false
                                    field.text = ""
                                    return
                                }
                            }
                            Apps.launchById(appCell.modelData.appId)
                            launcher.open = false
                            field.text = ""
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: appsGrid.count === 0
                    text: qsTr("Nothing found")
                    color: Theme.onSurfaceVariant
                    font.pixelSize: 14
                }

                ScrollIndicator {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                }
            }
        }
    }
}
