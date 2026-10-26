import QtQuick
import QtQuick.Controls
import QtQuick.Window
import Loali

Rectangle {
    id: pill

    signal settingsRequested()

    property var shell

    width: row.width + 28
    height: 64
    radius: 32
    color: Theme.surfaceContainer
    border.width: 1
    border.color: Qt.alpha(Theme.outlineVariant, 0.6)

    Shadow {
        extent: 10
        radius: 32
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 10

        Repeater {
            model: pill.shell ? pill.shell.workspaceCount : 0

            Item {
                id: wsDot
                required property int index
                width: 32
                height: 44
                readonly property bool current: pill.shell && wsDot.index === pill.shell.currentWorkspace

                Rectangle {
                    anchors.centerIn: parent
                    width: wsDot.current ? 22 : 10
                    height: 10
                    radius: 5
                    color: wsDot.current ? Theme.primary
                                        : dotHover.containsMouse ? Theme.onSurfaceVariant
                                        : Qt.alpha(Theme.onSurfaceVariant, 0.5)
                    Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutBack } }
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                MouseArea {
                    id: dotHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: pill.shell.switchWorkspace(wsDot.index)
                }

                ToolTip.visible: dotHover.containsMouse
                ToolTip.text: qsTr("Workspace %1").arg(wsDot.index + 1)
                ToolTip.delay: 400
            }
        }

        Rectangle {
            width: 1
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            visible: Tray.count > 0
            color: Qt.alpha(Theme.outlineVariant, 0.5)
        }

        Item {
            id: expoBtn
            width: 44
            height: 44
            scale: expoHover.containsMouse ? 1.1 : 1
            Behavior on scale {
                SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 40
                radius: 20
                color: expoHover.containsMouse ? Qt.alpha(Theme.onSurface, 0.1) : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            MIcon {
                anchors.centerIn: parent
                width: 22
                height: 22
                path: Theme.iconExpo
                color: Theme.onSurfaceVariant
            }

            MouseArea {
                id: expoHover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: pill.shell.toggleExpo()
            }

            ToolTip.visible: expoHover.containsMouse
            ToolTip.text: qsTr("Overview") + " (Meta+W)"
            ToolTip.delay: 400
        }

        Rectangle {
            width: 1
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            visible: Tray.count > 0
            color: Qt.alpha(Theme.outlineVariant, 0.5)
        }

        Item {
            id: trayRow
            width: Tray.count > 0 ? Tray.count * 44 - 10 : 0
            height: 44

            QtObject {
                id: trayDrag
                property int active: -1
                property real startX: 0
                property real pos: 0
                readonly property int target: active >= 0
                    ? Math.max(0, Math.min(Tray.count - 1, Math.round(pos / 44)))
                    : -1
            }

            Repeater {
                model: Tray

                Item {
                    id: trayItem
                    required property int index
                    required property string uid
                    required property string title
                    required property string iconName
                    required property int iconSerial
                    required property string status
                    required property string menuPath
                    width: 34
                    height: 44
                    opacity: status === "Passive" ? 0.55 : 1
                    z: trayDrag.active === trayItem.index ? 10 : 0
                    x: {
                        const i = trayItem.index
                        if (trayDrag.active === i)
                            return Math.max(0, Math.min((Tray.count - 1) * 44, trayDrag.pos))
                        if (trayDrag.active >= 0) {
                            const t = trayDrag.target
                            if (trayDrag.active < i && i <= t)
                                return (i - 1) * 44
                            if (trayDrag.active > i && i >= t)
                                return (i + 1) * 44
                        }
                        return i * 44
                    }
                    Behavior on x {
                        enabled: trayDrag.active !== trayItem.index
                        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                    }
                    Component.onDestruction: {
                        if (trayDrag.active === trayItem.index)
                            trayDrag.active = -1
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 30
                        height: 30
                        radius: 15
                        color: trayHover.containsMouse ? Qt.alpha(Theme.onSurface, 0.1) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 20
                        height: 20
                        source: trayItem.iconName !== "" ? "image://loali/" + trayItem.iconName
                                                         : "image://sni/" + trayItem.uid + "?" + trayItem.iconSerial
                        sourceSize: Qt.size(40, 40)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    MouseArea {
                        id: trayHover
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                        property real pressX: 0
                        property real pressY: 0
                        property bool dragged: false
                        onPressed: (mouse) => {
                            pressX = mouse.x
                            pressY = mouse.y
                            dragged = false
                        }
                        onPositionChanged: (mouse) => {
                            if (!(mouse.buttons & Qt.LeftButton))
                                return
                            if (trayDrag.active >= 0 && trayDrag.active !== trayItem.index)
                                return
                            if (!dragged) {
                                const dx = mouse.x - pressX
                                const dy = mouse.y - pressY
                                if (dx * dx + dy * dy < 49)
                                    return
                                dragged = true
                                const p = trayItem.mapToItem(trayRow, 0, 0)
                                trayDrag.startX = p.x
                                trayDrag.pos = p.x
                                trayDrag.active = trayItem.index
                            }
                            trayDrag.pos = trayDrag.startX + (mouse.x - pressX)
                        }
                        onReleased: (mouse) => {
                            if (!dragged)
                                return
                            if (trayDrag.target >= 0 && trayDrag.target !== trayItem.index)
                                Tray.move(trayItem.index, trayDrag.target)
                            trayDrag.active = -1
                        }
                        onClicked: (mouse) => {
                            if (dragged) {
                                dragged = false
                                return
                            }
                            const p = trayItem.mapToItem(null, 0, 0)
                            const w = pill.Window.window
                            const gx = (w ? w.x : 0) + p.x + 17
                            const gy = (w ? w.y : 0) + p.y + 22
                            if (mouse.button === Qt.RightButton) {
                                if (!(pill.shell && pill.shell.openTrayMenu(trayItem.index, p.x + 17, p.y + 56)))
                                    Tray.contextMenu(trayItem.index, gx, gy)
                            } else if (mouse.button === Qt.MiddleButton)
                                Tray.secondaryActivate(trayItem.index, gx, gy)
                            else
                                Tray.activate(trayItem.index, gx, gy)
                        }
                    }

                    ToolTip.visible: trayHover.containsMouse && trayItem.title !== "" && trayDrag.active < 0
                    ToolTip.text: trayItem.title
                    ToolTip.delay: 400
                }
            }
        }

        Rectangle {
            width: 1
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            color: Qt.alpha(Theme.outlineVariant, 0.5)
        }

        Item {
            id: themeBtn
            width: 44
            height: 44
            scale: themeHover.containsMouse ? 1.1 : 1
            Behavior on scale {
                SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 40
                radius: 20
                color: themeHover.containsMouse ? Qt.alpha(Theme.onSurface, 0.1) : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            MIcon {
                anchors.centerIn: parent
                width: 22
                height: 22
                path: Settings.dark ? Theme.iconSun : Theme.iconMoon
                color: Theme.onSurfaceVariant
                rotation: Settings.dark ? 0 : 180
                Behavior on rotation { NumberAnimation { duration: 300; easing.type: Easing.OutBack } }
            }

            MouseArea {
                id: themeHover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: Settings.dark = !Settings.dark
            }

            ToolTip.visible: themeHover.containsMouse
            ToolTip.text: Settings.dark ? qsTr("Light theme") : qsTr("Dark theme")
            ToolTip.delay: 400
        }

        Item {
            id: gearBtn
            width: 44
            height: 44
            scale: gearHover.containsMouse ? 1.1 : 1
            Behavior on scale {
                SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 40
                radius: 20
                color: gearHover.containsMouse ? Qt.alpha(Theme.onSurface, 0.1) : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            MIcon {
                anchors.centerIn: parent
                width: 22
                height: 22
                path: Theme.iconSettings
                color: Theme.onSurfaceVariant
            }

            MouseArea {
                id: gearHover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: pill.settingsRequested()
            }

            ToolTip.visible: gearHover.containsMouse
            ToolTip.text: qsTr("Settings")
            ToolTip.delay: 400
        }
    }
}
