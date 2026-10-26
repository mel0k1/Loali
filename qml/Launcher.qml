import QtQuick
import QtQuick.Controls
import Loali

Item {
    id: launcher

    property bool open: false

    anchors.fill: parent
    opacity: open ? 1 : 0
    visible: opacity > 0
    enabled: open
    Behavior on opacity { NumberAnimation { duration: 180 } }

    function filtered() {
        return Apps.filterApps(field.text)
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
        width: Math.min(680, launcher.width * 0.86)
        height: Math.min(560, launcher.height * 0.86)
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
            spacing: 16

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
                height: parent.height - 68
                cellWidth: 104
                cellHeight: 112
                clip: true
                model: Apps.filterApps(field.text)

                delegate: Item {
                    id: appCell
                    required property var modelData
                    width: appsGrid.cellWidth
                    height: appsGrid.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        radius: 18
                        color: hover.containsMouse ? Qt.alpha(Theme.onSurface, 0.08) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 16
                        width: 52
                        height: 52
                        source: "image://loali/" + appCell.modelData.icon
                        sourceSize: Qt.size(104, 104)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 10
                        width: parent.width - 12
                        text: appCell.modelData.name
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        color: Theme.onSurface
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: hover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
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
            }
        }
    }
}
