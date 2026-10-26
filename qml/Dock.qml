import QtQuick
import QtQuick.Controls
import Loali

Rectangle {
    id: dock

    signal launcherToggled()

    property var shell
    property bool launcherOpen: false

    function winsFor(appId) {
        return shell.windowsForApp(appId)
    }

    function runningFor(appId) {
        return winsFor(appId).length > 0
    }

    function activeFor(appId) {
        const wins = winsFor(appId)
        for (let i = 0; i < wins.length; ++i) {
            if (wins[i].active && !wins[i].minimized)
                return true
        }
        return false
    }

    function activateApp(entry) {
        const wins = winsFor(entry.appId)
        if (wins.length === 0) {
            Apps.launchById(entry.appId)
            return
        }
        const w = wins[0]
        if (w.active && !w.minimized)
            shell.minimize(w)
        else
            shell.restore(w)
    }

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
        spacing: 8

        DockButton {
            glyph: Theme.iconApps
            tip: qsTr("Apps")
            engaged: dock.launcherOpen
            onClicked: dock.launcherToggled()
        }

        Rectangle {
            width: 1
            height: 32
            anchors.verticalCenter: parent.verticalCenter
            color: Qt.alpha(Theme.outlineVariant, 0.5)
        }

        Repeater {
            model: Apps.pinnedApps

            DockButton {
                required property var modelData
                icon: modelData.icon
                tip: modelData.name
                running: dock.runningFor(modelData.appId)
                engaged: dock.activeFor(modelData.appId)
                onClicked: dock.activateApp(modelData)

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: Apps.togglePin(modelData.appId)
                }
            }
        }
    }

    component DockButton: Item {
        id: btn
        property string glyph
        property string icon
        property string tip
        property bool running: false
        property bool engaged: false
        signal clicked()
        width: 48
        height: 48
        scale: ma.containsMouse ? 1.12 : 1
        Behavior on scale {
            SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
        }
        Rectangle {
            anchors.centerIn: parent
            width: 44
            height: 44
            radius: 22
            color: btn.engaged ? Theme.primary
                 : ma.containsMouse ? Qt.alpha(Theme.onSurface, 0.1)
                 : "transparent"
            Behavior on color { ColorAnimation { duration: 150 } }
        }
        MIcon {
            anchors.centerIn: parent
            width: 24
            height: 24
            visible: btn.glyph !== ""
            path: btn.glyph
            color: btn.engaged ? Theme.onPrimary : Theme.onSurfaceVariant
        }
        Image {
            anchors.centerIn: parent
            width: 38
            height: 38
            visible: btn.icon !== ""
            source: "image://loali/" + btn.icon
            sourceSize: Qt.size(76, 76)
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height - 3
            width: btn.engaged ? 16 : 5
            height: 4
            radius: 2
            color: Theme.primary
            visible: btn.running
            Behavior on width { NumberAnimation { duration: 200 } }
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            onClicked: btn.clicked()
        }
        ToolTip.visible: ma.containsMouse && btn.tip !== ""
        ToolTip.text: btn.tip
        ToolTip.delay: 400
    }
}
