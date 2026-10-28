import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtWayland.Compositor
import Loali

Rectangle {
    antialiasing: true
    id: dock

    signal launcherToggled()
    signal lockRequested()
    signal settingsRequested()

    property var shell
    property var windows
    property var wallpaperItem: null
    property bool launcherOpen: false
    readonly property bool plane: Settings.dockMode === Settings.Plane
    readonly property bool labeled: Settings.taskbarStyle === Settings.Full
    readonly property int pad: plane ? 16 : 12
    readonly property int trayPitch: Theme.sz(36)
    property string pulseText: ""
    readonly property bool pulsing: pulseTimer.running

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
            const btn = pinnedButtonFor(entry.appId)
            if (btn)
                btn.fireBurst()
            return
        }
        const w = wins[0]
        if (w.active && !w.minimized)
            shell.minimize(w)
        else
            shell.restore(w)
    }

    function pinnedButtonFor(appId) {
        for (let i = 0; i < appsRow.children.length; ++i) {
            const it = appsRow.children[i]
            if (it.appRef === appId)
                return it
        }
        for (let j = 0; j < appsRow.children.length; ++j) {
            const row = appsRow.children[j]
            if (!row.children)
                continue
            for (let k = 0; k < row.children.length; ++k) {
                const inner = row.children[k]
                if (inner.appRef === appId)
                    return inner
            }
        }
        return null
    }

    function buttonFor(w) {
        for (let i = 0; i < chipsRow.children.length; ++i) {
            const it = chipsRow.children[i]
            if (it.winRef === w)
                return it
        }
        return null
    }

    function showPreviewFor(w) {
        if (!w || !w.surface) {
            hidePreview()
            return
        }
        const btn = buttonFor(w)
        const pw = 264
        if (btn && parent) {
            const cx = btn.mapToItem(dock, btn.width / 2, 0).x
            previewX = Math.max(8, Math.min(dock.width - pw - 8, cx - pw / 2))
        } else {
            previewX = Math.round((dock.width - pw) / 2)
        }
        previewWin = w
    }

    function hidePreview() {
        previewWin = null
    }

    implicitWidth: appsRow.width + statusRow.width + Theme.sz(14) + pad * 2
    radius: plane ? 0 : height / 2
    color: "transparent"
    border.width: plane ? 0 : 1
    border.color: Qt.alpha(Theme.outlineVariant, 0.6)
    clip: false

    WallpaperTint {
        anchors.fill: parent
        wallpaperItem: dock.wallpaperItem
        cornerRadius: dock.radius
        tintColor: Theme.surfaceContainer
        tintAlpha: dock.plane ? 0.62 : 0.55
    }

    Behavior on radius {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } }
    }

    Shadow {
        extent: 10
        radius: 32
        visible: !dock.plane
    }

    Rectangle {
        visible: dock.plane
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Qt.alpha(Theme.outlineVariant, 0.6)
    }

    Item {
        id: appsClip
        x: dock.pad
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(0, Math.min(appsRow.width, dock.width - dock.pad * 2 - statusRow.width - Theme.sz(14)))
        height: Theme.sz(46)
        clip: true

        Row {
            id: appsRow
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.sz(6)

            DockButton {
                glyph: Theme.iconApps
                tip: qsTr("Apps")
                engaged: dock.launcherOpen
                onClicked: dock.launcherToggled()
            }

            VDiv {}

            Repeater {
                model: Apps.pinnedApps

                DockButton {
                    required property var modelData
                    appRef: modelData.appId
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
                    NumberAnimation { duration: Theme.durSlow; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
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
                        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } }
                    }
                }
            }

            VDiv {
                visible: dock.windows && dock.windows.count > 0
            }

            Row {
                id: chipsRow
                spacing: Theme.sz(6)

                Repeater {
                    model: dock.windows

                    TaskButton {
                        required property var win
                        winRef: win
                    }
                }
            }
        }
    }

    Row {
        id: statusRow
        anchors.right: parent.right
        anchors.rightMargin: dock.pad
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.sz(8)

        Repeater {
            model: dock.shell ? dock.shell.workspaceCount : 0

            Item {
                id: wsDot
                required property int index
                width: Theme.sz(30)
                height: Theme.sz(38)
                readonly property bool current: dock.shell && wsDot.index === dock.shell.currentWorkspace

                Rectangle {
                    anchors.centerIn: parent
                    width: wsDot.current ? Theme.sz(20) : Theme.sz(9)
                    height: Theme.sz(9)
                    radius: width / 2
                    color: wsDot.current ? Theme.primary
                                        : dotHover.containsMouse ? Theme.onSurfaceVariant
                                        : Qt.alpha(Theme.onSurfaceVariant, 0.5)
                    Behavior on width { NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialFast } } }
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                MouseArea {
                    id: dotHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: dock.shell.switchWorkspace(wsDot.index)
                }

                ToolTip.visible: dotHover.containsMouse
                ToolTip.text: qsTr("Workspace %1").arg(wsDot.index + 1)
                ToolTip.delay: 400
            }
        }

        IconBtn {
            path: Theme.iconExpo
            tip: qsTr("Overview") + " (Meta+W)"
            onClicked: dock.shell.toggleExpo()
        }

        VDiv {
            visible: Tray.count > 0
        }

        Item {
            id: trayRow
            width: Tray.count > 0 ? Tray.count * dock.trayPitch - Theme.sz(8) : 0
            height: Theme.sz(38)

            QtObject {
                id: trayDrag
                property int active: -1
                property real startX: 0
                property real pos: 0
                readonly property int target: active >= 0
                    ? Math.max(0, Math.min(Tray.count - 1, Math.round(pos / dock.trayPitch)))
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
                    width: Theme.sz(28)
                    height: dock.trayPitch
                    opacity: status === "Passive" ? 0.55 : 1
                    z: trayDrag.active === trayItem.index ? 10 : 0
                    x: {
                        const i = trayItem.index
                        const p = dock.trayPitch
                        if (trayDrag.active === i)
                            return Math.max(0, Math.min((Tray.count - 1) * p, trayDrag.pos))
                        if (trayDrag.active >= 0) {
                            const t = trayDrag.target
                            if (trayDrag.active < i && i <= t)
                                return (i - 1) * p
                            if (trayDrag.active > i && i >= t)
                                return (i + 1) * p
                        }
                        return i * p
                    }
                    Behavior on x {
                        enabled: trayDrag.active !== trayItem.index
                        NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialFast } }
                    }
                    Component.onDestruction: {
                        if (trayDrag.active === trayItem.index)
                            trayDrag.active = -1
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: Theme.sz(26)
                        height: Theme.sz(26)
                        radius: width / 2
                        color: trayHover.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.1) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Image {
                        smooth: true
                        anchors.centerIn: parent
                        width: Theme.sz(18)
                        height: Theme.sz(18)
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
                            const w = dock.Window.window
                            const gx = (w ? w.x : 0) + p.x + trayItem.width / 2
                            const gy = (w ? w.y : 0) + p.y + trayItem.height / 2
                            if (mouse.button === Qt.RightButton) {
                                if (!(dock.shell && dock.shell.openTrayMenu(trayItem.index, p.x + trayItem.width / 2, p.y - 6, true)))
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

        VDiv {}

        IconBtn {
            path: Settings.dark ? Theme.iconSun : Theme.iconMoon
            tip: Settings.dark ? qsTr("Light theme") : qsTr("Dark theme")
            rotation: Settings.dark ? 0 : 180
            onClicked: Settings.dark = !Settings.dark
        }

        IconBtn {
            path: Theme.iconLock
            tip: qsTr("Lock")
            onClicked: dock.lockRequested()
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: clock
                anchors.horizontalCenter: parent.horizontalCenter
                text: Qt.formatDateTime(new Date(), "HH:mm")
                font.pixelSize: 13
                font.weight: Font.Medium
                color: Theme.textOnSurface
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

        IconBtn {
            path: Theme.iconSettings
            tip: qsTr("Settings")
            onClicked: dock.settingsRequested()
        }
    }

    Timer {
        id: pulseTimer
        interval: 1900
    }

    Timer {
        id: previewHide
        interval: 320
        onTriggered: dock.hidePreview()
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
            if (!dock.previewWin)
                return
            let found = false
            for (let i = 0; i < dock.shell.windows.count; ++i) {
                if (dock.shell.windows.get(i).win === dock.previewWin) {
                    found = true
                    break
                }
            }
            if (!found)
                dock.hidePreview()
        }
    }

    property int prevWins: 0

    Rectangle {
        id: preview
        parent: dock.parent
        visible: opacity > 0
        opacity: dock.previewWin !== null ? 1 : 0
        scale: opacity > 0.5 ? 1 : 0.94
        z: 120
        width: 264
        height: 218
        x: dock.x + dock.previewX
        y: dock.y - height - 10
        radius: 22
        color: Theme.surfaceContainerHigh
        border.width: 1
        border.color: Qt.alpha(Theme.outlineVariant, 0.7)
        clip: true

        Behavior on opacity { NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsFast } } }
        Behavior on x { NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialFast } } }
        Behavior on scale {
            NumberAnimation {
                duration: Theme.durFast
                easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialFast }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: previewHide.stop()
            onExited: previewHide.restart()
        }

        Item {
            id: previewHead
            x: 14
            y: 10
            width: parent.width - 28
            height: 28

            Image {
                smooth: true
                id: previewIcon
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                source: "image://loali/" + (dock.previewWin
                    ? Apps.iconForAppId(dock.previewWin.toplevel.appId) : "")
                sourceSize: Qt.size(36, 36)
                asynchronous: true
            }

            Text {
                anchors.left: previewIcon.right
                anchors.leftMargin: 8
                anchors.right: previewClose.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: dock.previewWin ? dock.previewWin.toplevel.title : ""
                elide: Text.ElideRight
                font.pixelSize: 12
                font.weight: Font.Medium
                color: Theme.textOnSurface
            }

            Item {
                id: previewClose
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26

                Rectangle {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    radius: 12
                    color: closeMa.containsMouse ? Theme.error : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                MIcon {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    path: Theme.iconClose
                    color: closeMa.containsMouse ? "#FFFFFF" : Theme.onSurfaceVariant
                }

                MouseArea {
                    id: closeMa
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: previewHide.stop()
                    onExited: previewHide.restart()
                    onClicked: {
                        if (dock.previewWin)
                            dock.previewWin.toplevel.sendClose()
                        dock.hidePreview()
                    }
                }
            }
        }

        Rectangle {
            id: shotArea
            x: 12
            y: 46
            width: parent.width - 24
            height: parent.height - 58
            radius: 14
            color: Theme.surfaceContainerLow
            clip: true

            Loader {
                anchors.fill: parent
                active: dock.previewWin !== null
                sourceComponent: previewSurfaceComp
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (dock.previewWin) {
                        dock.shell.restore(dock.previewWin)
                        dock.hidePreview()
                    }
                }
            }
        }
    }

    property var previewWin: null
    property real previewX: 0

    Component {
        id: previewSurfaceComp

        WaylandQuickItem {
            id: pv
            anchors.centerIn: parent
            readonly property real sw: dock.previewWin ? Math.max(dock.previewWin.width, 1) : 1
            readonly property real sh: dock.previewWin
                ? Math.max(dock.previewWin.height - dock.previewWin.barHeight, 1) : 1
            readonly property real fit: Math.min(shotArea.width / sw, shotArea.height / sh)
            width: sw * fit
            height: sh * fit
            surface: dock.previewWin && dock.previewWin.surface
                     ? dock.previewWin.surface : null
            output: dock.shell ? dock.shell.defaultOutput : null
            enabled: false
            focusOnClick: false
        }
    }

    component VDiv: Rectangle {
        width: 1
        height: Theme.sz(26)
        anchors.verticalCenter: parent.verticalCenter
        color: Qt.alpha(Theme.outlineVariant, 0.5)
    }

    component IconBtn: Item {
        id: ib
        property string path
        property string tip
        property real rotation: 0
        signal clicked()
        width: Theme.sz(38)
        height: Theme.sz(38)
        scale: ibMa.containsMouse ? 1.1 : 1
        Behavior on scale {
            SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
        }

        Rectangle {
            anchors.centerIn: parent
            width: Theme.sz(34)
            height: Theme.sz(34)
            radius: width / 2
            color: ibMa.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.1) : "transparent"
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        MIcon {
            anchors.centerIn: parent
            width: Theme.sz(20)
            height: Theme.sz(20)
            path: ib.path
            color: Theme.onSurfaceVariant
            rotation: ib.rotation
            Behavior on rotation { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } } }
        }

        MouseArea {
            id: ibMa
            anchors.fill: parent
            hoverEnabled: true
            onClicked: ib.clicked()
        }

        ToolTip.visible: ibMa.containsMouse && ib.tip !== ""
        ToolTip.text: ib.tip
        ToolTip.delay: 400
    }

    component DockButton: Item {
        id: btn
        property string glyph
        property string icon
        property string tip
        property string appRef
        property bool running: false
        property bool engaged: false
        signal clicked()
        width: Theme.sz(42)
        height: Theme.sz(42)
        scale: ma.containsMouse ? 1.12 : 1
        Behavior on scale {
            SpringAnimation { spring: 5; mass: 0.55; damping: 0.35 }
        }
        function fireBurst() {
            burstAnim.restart()
        }

        MorphShape {
            id: btnBg
            anchors.centerIn: parent
            width: Theme.sz(38)
            height: Theme.sz(38)
            fromShape: "square"
            toShape: "circle"
            progress: (ma.containsMouse || btn.engaged) ? 1 : 0
            color: btn.engaged ? Theme.primary
                 : ma.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.12)
                 : "transparent"
            Behavior on progress {
                NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
            }
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        ParallelAnimation {
            id: burstAnim
            running: false
            NumberAnimation { target: burstShape; property: "opacity"; from: 0.85; to: 0; duration: 560 }
            NumberAnimation { target: burstShape; property: "scale"; from: 0.3; to: 1.4; duration: 560; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialSlow } }
            NumberAnimation { target: burstShape; property: "progress"; from: 0; to: 1; duration: 560; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialSlow } }
        }

        MorphShape {
            id: burstShape
            anchors.centerIn: parent
            width: Theme.sz(46)
            height: Theme.sz(46)
            visible: opacity > 0
            opacity: 0
            fromShape: "boom"
            toShape: "flower"
            color: Theme.primary
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
            smooth: true
            anchors.centerIn: parent
            width: Theme.sz(32)
            height: Theme.sz(32)
            visible: btn.icon !== ""
            source: "image://loali/" + btn.icon
            sourceSize: Qt.size(64, 64)
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
        MorphShape {
            id: indicator
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height - 5
            width: btn.engaged ? Theme.sz(16) : Theme.sz(6)
            height: 3
            visible: btn.running
            fromShape: "circle"
            toShape: "pill"
            progress: btn.engaged ? 1 : 0
            color: Theme.primary
            Behavior on width { NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } } }
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

    component TaskButton: Rectangle {
        id: taskBtn
        property var winRef
        readonly property bool isActive: winRef ? (winRef.active && !winRef.minimized) : false
        readonly property bool labeled: dock.labeled
        width: labeled ? Math.min(150, label.width + 40) : Theme.sz(42)
        height: Theme.sz(36)
        radius: height / 2
        color: isActive ? Theme.secondaryContainer
             : taskMa.containsMouse ? Theme.surfaceContainerHighest
             : "transparent"
        opacity: winRef && winRef.minimized ? 0.5 : 1
        anchors.verticalCenter: parent.verticalCenter
        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on width { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } } }

        readonly property string appIcon: Apps.iconForAppId(taskBtn.winRef ? taskBtn.winRef.toplevel.appId : "")

        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: taskBtn.labeled ? 10 : (taskBtn.width - taskBtn.iconW) / 2
            spacing: 7

            readonly property real iconW: taskBtn.labeled ? 18 : 20

            Image {
                smooth: true
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                visible: taskBtn.appIcon !== ""
                source: "image://loali/" + taskBtn.appIcon
                sourceSize: Qt.size(36, 36)
                asynchronous: true
            }

            MIcon {
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
                visible: taskBtn.appIcon === ""
                path: Theme.iconApps
                color: Theme.onSurfaceVariant
            }

            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, 96)
                visible: taskBtn.labeled
                text: taskBtn.winRef ? taskBtn.winRef.toplevel.title : ""
                elide: Text.ElideRight
                font.pixelSize: 12
                color: Theme.textOnSurface
            }
        }

        MorphShape {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -1
            width: taskBtn.isActive ? Theme.sz(16) : Theme.sz(6)
            height: 3
            visible: !taskBtn.labeled && (taskBtn.isActive || (taskBtn.winRef && !taskBtn.winRef.minimized))
            fromShape: "circle"
            toShape: "pill"
            progress: taskBtn.isActive ? 1 : 0
            Behavior on progress {
                NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } }
            }
            color: Theme.primary
            Behavior on width { NumberAnimation { duration: Theme.durFast; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } } }
        }

        MouseArea {
            id: taskMa
            anchors.fill: parent
            hoverEnabled: true
            onContainsMouseChanged: {
                if (containsMouse) {
                    previewHide.stop()
                    dock.showPreviewFor(taskBtn.winRef)
                } else if (dock.previewWin === taskBtn.winRef) {
                    previewHide.restart()
                }
            }
            onClicked: {
                const w = taskBtn.winRef
                if (w.active && !w.minimized)
                    dock.shell.minimize(w)
                else
                    dock.shell.restore(w)
                dock.hidePreview()
            }
        }
    }
}
