import QtQuick
import QtQuick.Controls
import Loali

Rectangle {
    id: taskbar

    signal lockRequested()

    property var shell
    property var windows

    height: 64
    color: Theme.surfaceContainer

    Rectangle {
        width: parent.width
        height: 1
        color: Qt.alpha(Theme.outlineVariant, 0.6)
    }

    Row {
        id: tasks
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Repeater {
            model: taskbar.windows

            TaskButton {
                required property var win
                winRef: win
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 14

        Item {
            width: 40
            height: 40
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                anchors.centerIn: parent
                width: 36
                height: 36
                radius: 18
                color: lockMa.containsMouse ? Qt.alpha(Theme.onSurface, 0.1) : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
            }
            MIcon {
                anchors.centerIn: parent
                width: 20
                height: 20
                path: Theme.iconLock
                color: Theme.onSurfaceVariant
            }
            MouseArea {
                id: lockMa
                anchors.fill: parent
                hoverEnabled: true
                onClicked: taskbar.lockRequested()
            }
            ToolTip.visible: lockMa.containsMouse
            ToolTip.text: qsTr("Lock")
            ToolTip.delay: 400
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: clock
                anchors.horizontalCenter: parent.horizontalCenter
                text: Qt.formatDateTime(new Date(), "HH:mm")
                font.pixelSize: 15
                font.weight: Font.Medium
                color: Theme.onSurface
            }

            Text {
                id: dateText
                anchors.horizontalCenter: parent.horizontalCenter
                text: Qt.formatDate(new Date(), "ddd d MMM")
                font.pixelSize: 11
                color: Theme.onSurfaceVariant
            }

            Timer {
                interval: 5000
                running: true
                repeat: true
                onTriggered: {
                    clock.text = Qt.formatDateTime(new Date(), "HH:mm")
                    dateText.text = Qt.formatDate(new Date(), "ddd d MMM")
                }
            }
        }
    }

    component TaskButton: Rectangle {
        id: taskBtn
        property var winRef
        readonly property bool isActive: winRef ? (winRef.active && !winRef.minimized) : false
        width: Math.min(190, label.width + 46)
        height: 42
        radius: 21
        color: isActive ? Theme.secondaryContainer
             : taskMa.containsMouse ? Theme.surfaceContainerHighest
             : "transparent"
        opacity: winRef && winRef.minimized ? 0.5 : 1
        Behavior on color { ColorAnimation { duration: 150 } }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: 12
            spacing: 8

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 20
                height: 20
                source: "image://loali/" + Apps.iconForAppId(taskBtn.winRef ? taskBtn.winRef.toplevel.appId : "")
                sourceSize: Qt.size(40, 40)
                asynchronous: true
            }

            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, 120)
                text: taskBtn.winRef ? taskBtn.winRef.toplevel.title : ""
                elide: Text.ElideRight
                font.pixelSize: 13
                color: Theme.onSurface
            }
        }

        MouseArea {
            id: taskMa
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                const w = taskBtn.winRef
                if (w.active && !w.minimized)
                    taskbar.shell.minimize(w)
                else
                    taskbar.shell.restore(w)
            }
        }
    }
}
