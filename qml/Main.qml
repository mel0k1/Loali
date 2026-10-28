import QtQuick
import QtQuick.Window
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell
import Loali

WaylandCompositor {
    id: comp


    readonly property int dockHeight: Theme.sz(Settings.dockMode === Settings.Plane ? 56 : 52)
    readonly property bool dockPlane: Settings.dockMode === Settings.Plane
    readonly property real topInset: 10
    readonly property real bottomInset: dockPlane ? dockHeight : 12 + dockHeight + 8
    property int zTop: 10
    property bool locked: false
    property bool expoActive: false
    property int workspaceCount: Settings.workspaces
    property int currentWorkspace: 0
    property var activeWin: null
    property string snapMode: ""
    property bool desktopShown: false
    property bool debugLockLayer: false
    property alias xwayland: xwaylandShell
    readonly property alias blurSourceItem: blurContent
    property ListModel windows: ListModel {}

    onWorkspaceCountChanged: {
        if (currentWorkspace >= workspaceCount)
            currentWorkspace = workspaceCount - 1
        for (let i = 0; i < windows.count; ++i) {
            const w = windows.get(i).win
            if (w.ws >= workspaceCount)
                w.ws = workspaceCount - 1
        }
    }

    function maxArea() {
        const x = 10 + layerInsetLeft
        const y = topInset + layerInsetTop
        const w = win.width - x - (10 + layerInsetRight)
        const h = win.height - y - (bottomInset + layerInsetBottom)
        return Qt.rect(x, y, Math.max(120, w), Math.max(120, h))
    }

    readonly property var layerWins: []
    property int layerInsetLeft: 0
    property int layerInsetTop: 0
    property int layerInsetRight: 0
    property int layerInsetBottom: 0

    function recomputeLayerInsets() {
        let l = 0
        let t = 0
        let r = 0
        let b = 0
        for (let i = 0; i < layerWins.length; ++i) {
            const s = layerWins[i]
            if (!s)
                continue
            l = Math.max(l, s.edgeInsetLeft)
            t = Math.max(t, s.edgeInsetTop)
            r = Math.max(r, s.edgeInsetRight)
            b = Math.max(b, s.edgeInsetBottom)
        }
        layerInsetLeft = l
        layerInsetTop = t
        layerInsetRight = r
        layerInsetBottom = b
    }

    readonly property bool sessionLocked: sessionLock.locked

    function canOp() {
        return !locked && !expoActive && !sessionLocked
    }

    function toggleExpo() {
        launcher.open = false
        settingsApp.open = false
        trayMenuPanel.close()
        expo.toggle()
    }

    function fullArea() {
        return Qt.rect(0, 0, win.width, win.height)
    }

    function activate(c) {
        if (c.ws !== currentWorkspace)
            currentWorkspace = c.ws
        for (let i = 0; i < windows.count; ++i) {
            const w = windows.get(i).win
            w.active = (w === c)
        }
        activeWin = c
        if (!c.minimized)
            c.z = ++zTop
        Util.focus(defaultSeat, c.surface)
    }

    function deactivateAll() {
        for (let i = 0; i < windows.count; ++i)
            windows.get(i).win.active = false
        activeWin = null
        Util.focus(defaultSeat, null)
    }

    function topMost() {
        let best = null
        for (let i = 0; i < windows.count; ++i) {
            const w = windows.get(i).win
            if (w.ws !== currentWorkspace || w.minimized)
                continue
            if (!best || w.z > best.z)
                best = w
        }
        return best
    }

    function activeWindow() {
        if (activeWin && !activeWin.minimized && activeWin.ws === currentWorkspace)
            return activeWin
        return topMost()
    }

    function minimize(c) {
        c.minimized = true
        if (c.active) {
            c.active = false
            activeWin = null
            Util.focus(defaultSeat, null)
        }
    }

    function restore(c) {
        c.minimized = false
        desktopShown = false
        activate(c)
    }

    function switchWorkspace(i) {
        const n = Math.max(0, Math.min(workspaceCount - 1, i))
        if (n === currentWorkspace)
            return
        currentWorkspace = n
        const t = topMost()
        if (t)
            activate(t)
        else
            deactivateAll()
    }

    function sendToWorkspace(c, i) {
        const n = Math.max(0, Math.min(workspaceCount - 1, i))
        if (c.ws === n)
            return
        c.ws = n
        if (expoActive) {
            const h = expo.holderOf(n)
            if (h)
                c.parent = h
        }
        if (n === currentWorkspace) {
            activate(c)
            return
        }
        if (c.active) {
            c.active = false
            activeWin = null
            Util.focus(defaultSeat, null)
            const t = topMost()
            if (t)
                activate(t)
        }
    }

    function toggleShowDesktop() {
        if (!desktopShown) {
            for (let i = 0; i < windows.count; ++i) {
                const w = windows.get(i).win
                if (w.ws === currentWorkspace && !w.minimized)
                    minimize(w)
            }
            desktopShown = true
        } else {
            desktopShown = false
            const t = topMost()
            if (t)
                restore(t)
        }
    }

    function removeWindow(c) {
        for (let i = 0; i < windows.count; ++i) {
            if (windows.get(i).win === c) {
                windows.remove(i)
                break
            }
        }
        if (activeWin === c)
            activeWin = null
        Qt.callLater(c.destroy)
    }

    function windowsForApp(appId) {
        let res = []
        for (let i = 0; i < windows.count; ++i) {
            const w = windows.get(i).win
            if (matchApp(w.toplevel.appId, appId))
                res.push(w)
        }
        return res
    }

    function matchApp(appId, target) {
        const a = appId.toLowerCase().replace(/\.desktop$/, "")
        const b = target.toLowerCase().replace(/\.desktop$/, "")
        return a !== "" && b !== "" && (a === b || a.indexOf(b) >= 0 || b.indexOf(a) >= 0)
    }

    function showSnapPreview(mode) {
        const a = maxArea()
        snapMode = mode
        if (mode === "left") {
            snapPreview.x = a.x
            snapPreview.y = a.y
            snapPreview.width = a.width / 2
            snapPreview.height = a.height
        } else if (mode === "right") {
            snapPreview.x = a.x + a.width / 2
            snapPreview.y = a.y
            snapPreview.width = a.width / 2
            snapPreview.height = a.height
        } else {
            snapPreview.x = a.x
            snapPreview.y = a.y
            snapPreview.width = a.width
            snapPreview.height = a.height
        }
        snapPreview.opacity = 1
    }

    function hideSnapPreview() {
        snapMode = ""
        snapPreview.opacity = 0
    }

    function openTrayMenu(row, x, y, above) {
        launcher.open = false
        return trayMenuPanel.openAt(row, x, y, above === true)
    }

    function paletteSource() {
        if (Settings.autoAccent)
            MatColor.generateFromImage(Settings.wallpaper)
        else
            MatColor.generateFromColor(Settings.accent)
    }

    Component.onCompleted: {
        paletteSource()
        const args = Qt.application.arguments
        for (let i = 0; i < args.length; ++i) {
            if (args[i] === "--demo" && i + 1 < args.length)
                demoTimer.mode = args[i + 1]
            if (args[i] === "--demo-delay" && i + 1 < args.length)
                demoTimer.interval = parseInt(args[i + 1]) || 3200
        }
        if (demoTimer.mode !== "")
            demoTimer.restart()
        console.info("Loali socket:", socketName === "" ? "wayland-?" : socketName)
    }

    Timer {
        id: demoTimer
        repeat: false
        property string mode: ""
        onTriggered: {
            if (mode === "launcher")
                launcher.open = true
            else if (mode === "settings")
                settingsApp.open = true
            else if (mode === "shapes") {
                settingsApp.open = true
                settingsApp.page = 5
            }
            else if (mode === "expo")
                comp.toggleExpo()
            else if (mode === "preview") {
                const t = comp.topMost()
                if (t)
                    dockPanel.showPreviewFor(t)
            }
        }
    }

    Connections {
        target: Apps
        function onBuiltinRequested(id) {
            if (id.toLowerCase().indexOf("settings") >= 0) {
                launcher.open = false
                settingsApp.open = true
            }
        }
    }

    Connections {
        target: Settings
        function onWallpaperChanged() {
            if (Settings.autoAccent)
                MatColor.generateFromImage(Settings.wallpaper)
        }
        function onAutoAccentChanged() {
            paletteSource()
        }
        function onAccentChanged() {
            if (!Settings.autoAccent)
                MatColor.generateFromColor(Settings.accent)
        }
        function onColorStyleChanged() {
            MatColor.setStyle(Settings.colorStyle)
        }
        function onContrastChanged() {
            MatColor.setContrast(Settings.contrast)
        }
        function onPanelScaleChanged() {
            comp.reapplyLayouts()
        }
        function onDockModeChanged() {
            comp.reapplyLayouts()
        }
    }

    function reapplyLayouts() {
        for (let i = 0; i < windows.count; ++i)
            windows.get(i).win.reapplyLayout()
    }

    WaylandOutput {
        sizeFollowsWindow: true
        window: Window {
            id: win
            width: 1280
            height: 800
            visible: true
            title: "Loali"
            color: Theme.surface
            visibility: fullscreenMode ? Window.FullScreen : Window.AutomaticVisibility
            onClosing: Qt.quit()

            Item {
                id: blurContent
                anchors.fill: parent

                Desktop {
                    id: desktop
                    anchors.fill: parent
                    visible: !comp.sessionLocked
                    topInset: comp.topInset
                    bottomInset: comp.bottomInset
                    onEmptyClicked: comp.deactivateAll()
                }

                Item {
                    id: layerUnder
                    anchors.fill: parent
                }

                Item {
                    id: workspace
                    anchors.fill: parent
                    visible: !comp.sessionLocked
                }
            }

            Item {
                id: layerTopItem
                anchors.fill: parent
                z: 95
            }

            Item {
                id: layerOverlayItem
                anchors.fill: parent
                z: 450
            }

            Rectangle {
                antialiasing: true
                id: snapPreview
                visible: opacity > 0
                opacity: 0
                z: 90
                radius: 18
                color: Qt.alpha(Theme.primary, 0.22)
                border.width: 2
                border.color: Theme.primary
                Behavior on opacity { NumberAnimation { duration: 140 } }
                Behavior on x { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } } }
                Behavior on y { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } } }
                Behavior on width { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } } }
                Behavior on height { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } } }
            }

            BlurBackdrop {
                id: dockBlur
                x: dockPanel.x
                y: dockPanel.y
                width: dockPanel.width
                height: dockPanel.height
                cornerRadius: dockPanel.radius
                sourceItem: blurContent
                z: 99
                visible: !comp.sessionLocked
            }

            Dock {
                id: dockPanel
                shell: comp
                windows: comp.windows
                wallpaperItem: desktop.wallpaperItem
                launcherOpen: launcher.open
                visible: !comp.sessionLocked
                x: comp.dockPlane ? 0 : Math.round((win.width - width) / 2)
                y: comp.dockPlane ? win.height - height : win.height - height - 12
                width: comp.dockPlane ? win.width : Math.min(win.width - 16, implicitWidth)
                height: comp.dockHeight
                z: 100
                Behavior on x {
                    enabled: !comp.dockPlane
                    NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
                }
                Behavior on y {
                    NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
                }
                onLauncherToggled: launcher.open = !launcher.open
                onLockRequested: comp.locked = true
                onSettingsRequested: {
                    launcher.open = false
                    settingsApp.open = true
                }
            }

            Launcher {
                id: launcher
                shell: comp
                wallpaperItem: desktop.wallpaperItem
                z: 200
            }

            TrayMenuPanel {
                id: trayMenuPanel
                shell: comp
                wallpaperItem: desktop.wallpaperItem
                z: 210
            }

            Expo {
                id: expo
                shell: comp
                winLayer: workspace
                z: 150
            }

            NotificationPopups { wallpaperItem: desktop.wallpaperItem }

            SettingsApp {
                id: settingsApp
                wallpaperItem: desktop.wallpaperItem
                z: 200
            }

            LockScreen {
                locked: comp.locked && !comp.sessionLocked
                onUnlocked: comp.locked = false
            }

            Item {
                id: sessionLockLayer
                anchors.fill: parent
                z: 700
                visible: comp.sessionLocked

                Rectangle {
                    anchors.fill: parent
                    color: "#000000"
                }

                Repeater {
                    model: sessionLock.locked ? sessionLock.lockSurfaces : []

                    delegate: SessionLockView {
                        required property var modelData
                        anchors.fill: parent
                        lockSurface: modelData
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                z: 600
                color: "black"
                visible: Idle.blanked
            }

            Shortcut {
                sequence: "Ctrl+Alt+L"
                onActivated: comp.locked = !comp.locked
            }

            Shortcut {
                sequence: "Escape"
                enabled: (launcher.open || settingsApp.open || trayMenuPanel.openState || expo.active) && !comp.locked
                onActivated: {
                    if (trayMenuPanel.openState)
                        trayMenuPanel.close()
                    else if (expo.active)
                        expo.close()
                    else if (settingsApp.open)
                        settingsApp.open = false
                    else
                        launcher.open = false
                }
            }

            Shortcut {
                sequence: "Meta+W"
                enabled: !comp.locked
                onActivated: comp.toggleExpo()
            }

            Shortcut {
                sequence: "Meta+A"
                enabled: comp.canOp()
                onActivated: {
                    settingsApp.open = false
                    launcher.open = !launcher.open
                }
            }

            Shortcut {
                sequence: "Meta+S"
                enabled: comp.canOp()
                onActivated: {
                    launcher.open = false
                    settingsApp.open = !settingsApp.open
                }
            }

            Shortcut {
                sequence: "Meta+D"
                enabled: comp.canOp()
                onActivated: comp.toggleShowDesktop()
            }

            Shortcut {
                sequence: "Alt+F4"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.toplevel.sendClose()
                }
            }

            Shortcut {
                sequence: "Meta+F"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (!w)
                        return
                    if (w.fullscreen)
                        w.exitFullscreen()
                    else
                        w.enterFullscreen()
                }
            }

            Shortcut {
                sequence: "Meta+Up"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("max")
                }
            }

            Shortcut {
                sequence: "Meta+Down"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (!w)
                        return
                    if (w.maximized)
                        w.toggleMaximize()
                    else if (w.tiled !== "")
                        w.unsnap()
                    else
                        comp.minimize(w)
                }
            }

            Shortcut {
                sequence: "Meta+Left"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("left")
                }
            }

            Shortcut {
                sequence: "Meta+Right"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("right")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Left"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("tl")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Right"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("tr")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Up"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("bl")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Down"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("br")
                }
            }

            Shortcut {
                sequence: "Meta+1"
                enabled: comp.canOp()
                onActivated: comp.switchWorkspace(0)
            }
            Shortcut {
                sequence: "Meta+2"
                enabled: comp.canOp() && comp.workspaceCount > 1
                onActivated: comp.switchWorkspace(1)
            }
            Shortcut {
                sequence: "Meta+3"
                enabled: comp.canOp() && comp.workspaceCount > 2
                onActivated: comp.switchWorkspace(2)
            }
            Shortcut {
                sequence: "Meta+4"
                enabled: comp.canOp() && comp.workspaceCount > 3
                onActivated: comp.switchWorkspace(3)
            }
            Shortcut {
                sequence: "Meta+5"
                enabled: comp.canOp() && comp.workspaceCount > 4
                onActivated: comp.switchWorkspace(4)
            }
            Shortcut {
                sequence: "Meta+6"
                enabled: comp.canOp() && comp.workspaceCount > 5
                onActivated: comp.switchWorkspace(5)
            }

            Shortcut {
                sequence: "Meta+Shift+1"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 0)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+2"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 1)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+3"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 2)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+4"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 3)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+5"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 4)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+6"
                enabled: comp.canOp()
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 5)
                }
            }
        }
    }

    XdgShell {
        onToplevelCreated: (toplevel, xdgSurface) => {
            const c = chromeComponent.createObject(workspace, {
                shellSurface: xdgSurface,
                toplevel: toplevel,
                compositor: comp,
                ws: comp.currentWorkspace
            })
            windows.append({ win: c })
            c.windowClosed.connect(() => removeWindow(c))
            activate(c)
        }
    }

    LayerShell {
        onLayerSurfaceCreated: (surf) => {
            layerWins.push(surf)
            const holder = surf.layer <= 1 ? layerUnder
                         : (surf.layer === 2 ? layerTopItem : layerOverlayItem)
            const item = layerChromeComponent.createObject(holder, {
                shellSurface: surf
            })
            surf.surfaceClosed.connect(() => {
                const idx = layerWins.indexOf(surf)
                if (idx >= 0)
                    layerWins.splice(idx, 1)
                if (item)
                    item.destroy()
                comp.recomputeLayerInsets()
            })
            surf.placementChanged.connect(() => comp.recomputeLayerInsets())
            surf.stateChanged.connect(() => comp.recomputeLayerInsets())
            comp.recomputeLayerInsets()
        }
    }

    XWaylandShell {
        id: xwaylandShell
        onXwaylandSurfaceCreated: (surf) => {
            const c = xwaylandChromeComponent.createObject(workspace, {
                shellSurface: surf,
                compositor: comp,
                ws: comp.currentWorkspace
            })
            windows.append({ win: c })
            c.windowClosed.connect(() => removeWindow(c))
            surf.windowClosed.connect(() => removeWindow(c))
            comp.activate(c)
        }
    }

    XdgDecorationManagerV1 {
        preferredMode: XdgToplevel.ServerSideDecoration
    }

    XdgOutputManager { }

    IdleManager { }

    OutputPowerManager { }

    DataControlManager { }

    SessionLockManager {
        id: sessionLock
        onLockSurfacesChanged: console.info("loali: lock surfaces", lockSurfaces.length)
        onLockedChanged: console.info("loali: session lock", locked)
    }

    InputMethodManager { }

    VirtualKeyboardManager { }

    KdeBlurManager { }

    ExtBackgroundEffectManager { }

    ScreencopyManager { }

    WlOutputFix {
        compositor: comp
        Component.onCompleted: apply()
    }

    Component {
        id: chromeComponent
        Chrome {}
    }

    Component {
        id: layerChromeComponent
        LayerChrome {
            wallpaperItem: desktop.wallpaperItem
            blurSourceItem: blurContent
        }
    }

    Component {
        id: xwaylandChromeComponent
        XWaylandChrome {}
    }
}
