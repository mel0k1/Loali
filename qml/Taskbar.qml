import QtQuick
import QtQuick.Controls
import Loali

Rectangle {
    id: taskbar

    signal lockRequested()

    property var shell
    property var windows

    readonly property bool fullMode: Settings.taskbarStyle === Settings.Full
    readonly property bool stretched: fullMode || overflow
    readonly property int pad: 12
    readonly property real contentW: tasksPart.implicitWidth + rightPart.implicitWidth + 12 + pad * 2
    readonly property bool overflow: !fullMode && parent ? contentW > parent.width - 16 : false

    x: stretched ? 0 : Math.round((parent.width - width) / 2)
    y: stretched ? parent.height - height : parent.height - height - 12
    width: stretched ? parent.width : Math.min(parent.width - 16, contentW)
    height: Theme.sz(50)
    radius: stretched ? 0 : height / 2
    color: Theme.surfaceContainer
    border.width: stretched ? 0 : 1
    border.color: Qt.alpha(Theme.outlineVariant, 0.6)
    clip: true

    Behavior on x {
        NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
    }
    Behavior on y {
        NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
    }
    Behavior on width {
        NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
    }
    Behavior on radius {
        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
    }

    Shadow {
        extent: 10
        radius: 32
        visible: !taskbar.stretched
    }

    Rectangle {
        visible: taskbar.stretched
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Qt.alpha(Theme.outlineVariant, 0.6)
    }

    Row {
        id: barRow
        anchors.verticalCenter: parent.verticalCenter
        x: taskbar.stretched ? taskbar.pad : Math.round((taskbar.width - width) / 2)
        spacing: 10

        Behavior on x {
            NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
        }

        Row {
            id: tasksPart
            spacing: 6

            Repeater {
                model: taskbar.windows

                TaskButton {
                    required property var win
                    winRef: win
                }
            }
        }

        Item {
            width: taskbar.stretched
                ? Math.max(10, taskbar.width - taskbar.pad * 2 - tasksPart.width - rightPart.width)
                : 12
            height: 1
        }

        Row {
            id: rightPart
            spacing: 10

            Rectangle {
                width: 1
                height: Theme.sz(24)
                anchors.verticalCenter: parent.verticalCenter
                color: Qt.alpha(Theme.outlineVariant, 0.5)
            }

            Item {
                width: Theme.sz(34)
                height: Theme.sz(34)
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.sz(30)
                    height: Theme.sz(30)
                    radius: width / 2
                    color: lockMa.containsMouse ? Qt.alpha(Theme.onSurface, 0.1) : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                MIcon {
                    anchors.centerIn: parent
                    width: Theme.sz(18)
                    height: Theme.sz(18)
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
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.onSurface
                }

                Text {
                    id: dateText
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Qt.formatDate(new Date(), "ddd d MMM")
                    font.pixelSize: 10
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
    }

    component TaskButton: Rectangle {
        id: taskBtn
        property var winRef
        readonly property bool isActive: winRef ? (winRef.active && !winRef.minimized) : false
        width: Math.min(150, label.width + 40)
        height: Theme.sz(36)
        radius: height / 2
        color: isActive ? Theme.secondaryContainer
             : taskMa.containsMouse ? Theme.surfaceContainerHighest
             : "transparent"
        opacity: winRef && winRef.minimized ? 0.5 : 1
        anchors.verticalCenter: parent.verticalCenter
        Behavior on color { ColorAnimation { duration: 150 } }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: 10
            spacing: 7

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                source: "image://loali/" + Apps.iconForAppId(taskBtn.winRef ? taskBtn.winRef.toplevel.appId : "")
                sourceSize: Qt.size(36, 36)
                asynchronous: true
            }

            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, 96)
                text: taskBtn.winRef ? taskBtn.winRef.toplevel.title : ""
                elide: Text.ElideRight
                font.pixelSize: 12
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
