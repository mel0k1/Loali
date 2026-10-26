import QtQuick
import QtQuick.Controls
import Loali

Rectangle {
    id: dock

    signal launcherToggled()

    property var shell
    property bool launcherOpen: false
    readonly property bool plane: Settings.dockMode === Settings.Plane
    readonly property bool hot: hoverMa.containsMouse || launcherOpen || pulsing
    property string pulseText: ""
    readonly property bool pulsing: pulseTimer.running
    property int prevWins: 0

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

    implicitWidth: row.width + (plane ? 24 : hot ? Theme.sz(26) : Theme.sz(18))
    radius: plane ? 0 : height / 2
    color: Theme.surfaceContainer
    border.width: plane ? 0 : 1
    border.color: Qt.alpha(Theme.outlineVariant, 0.6)
    clip: false

    Behavior on width {
        enabled: !plane
        NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
    }
    Behavior on radius {
        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
    }

    Shadow {
        extent: 10
        radius: 32
        visible: !dock.plane
    }

    Rectangle {
        visible: dock.plane
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Qt.alpha(Theme.outlineVariant, 0.6)
    }

    MouseArea {
        id: hoverMa
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        x: dock.plane ? 12 : Math.round((dock.width - row.width) / 2)
        spacing: dock.plane ? Theme.sz(8) : dock.hot ? Theme.sz(10) : Theme.sz(6)

        Behavior on x {
            NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
        }
        Behavior on spacing {
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }

        DockButton {
            glyph: Theme.iconApps
            tip: qsTr("Apps")
            engaged: dock.launcherOpen
            onClicked: dock.launcherToggled()
        }

        Rectangle {
            width: 1
            height: Theme.sz(26)
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

        Item {
            width: dock.pulsing ? pulseLabel.width + 8 : 0
            height: parent.height
            clip: true
            visible: width > 0

            Behavior on width {
                NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
            }

            Text {
                id: pulseLabel
                anchors.verticalCenter: parent.verticalCenter
                x: 4
                text: dock.pulseText
                elide: Text.ElideRight
                width: Math.min(implicitWidth, 180)
                font.pixelSize: 12
                font.weight: Font.Medium
                color: Theme.primary
                opacity: dock.pulsing ? 1 : 0
                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
            }
        }
    }

    Timer {
        id: pulseTimer
        interval: 1900
    }

    Connections {
        target: dock.shell ? dock.shell.windows : null
        function onCountChanged() {
            const n = dock.shell.windows.count
            if (n > dock.prevWins && n > 0) {
                const w = dock.shell.windows.get(n - 1).win
                const t = w && w.toplevel ? w.toplevel.title : ""
                if (t !== "") {
                    dock.pulseText = t
                    pulseTimer.restart()
                }
            }
            dock.prevWins = n
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
        width: Theme.sz(42)
        height: Theme.sz(42)
        scale: ma.containsMouse ? 1.12 : 1
        Behavior on scale {
            SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
        }
        Rectangle {
            anchors.centerIn: parent
            width: Theme.sz(38)
            height: Theme.sz(38)
            radius: width / 2
            color: btn.engaged ? Theme.primary
                 : ma.containsMouse ? Qt.alpha(Theme.onSurface, 0.1)
                 : "transparent"
            Behavior on color { ColorAnimation { duration: 150 } }
        }
        MIcon {
            anchors.centerIn: parent
            width: Theme.sz(22)
            height: Theme.sz(22)
            visible: btn.glyph !== ""
            path: btn.glyph
            color: btn.engaged ? Theme.onPrimary : Theme.onSurfaceVariant
        }
        Image {
            anchors.centerIn: parent
            width: Theme.sz(32)
            height: Theme.sz(32)
            visible: btn.icon !== ""
            source: "image://loali/" + btn.icon
            sourceSize: Qt.size(64, 64)
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height - 2
            width: btn.engaged ? 14 : 4
            height: 3
            radius: 1.5
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
