import QtQuick
import QtQuick.Window
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell
import Loali

WaylandCompositor {
    id: comp

    readonly property int dockHeight: 64
    readonly property int taskbarHeight: 64
    readonly property real topInset: 12 + dockHeight + 10
    readonly property real bottomInset: taskbarHeight + 10
    property int zTop: 10
    property bool locked: false
    property int workspaceCount: Settings.workspaces
    property int currentWorkspace: 0
    property var activeWin: null
    property string snapMode: ""
    property bool desktopShown: false
    property ListModel windows: ListModel {}

    onWorkspaceCountChanged: {
        if (currentWorkspace >= workspaceCount)
            currentWorkspace = workspaceCount - 1
    }

    function maxArea() {
        return Qt.rect(10, topInset, win.width - 20, win.height - topInset - bottomInset)
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
        defaultSeat.setKeyboardFocus(c.content.surface)
    }

    function deactivateAll() {
        for (let i = 0; i < windows.count; ++i)
            windows.get(i).win.active = false
        activeWin = null
        defaultSeat.setKeyboardFocus(null)
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
            defaultSeat.setKeyboardFocus(null)
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
        if (n === currentWorkspace) {
            activate(c)
            return
        }
        if (c.active) {
            c.active = false
            activeWin = null
            defaultSeat.setKeyboardFocus(null)
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

    Component.onCompleted:
        console.info("Loali socket:", socketName === "" ? "wayland-?" : socketName)

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

            Desktop {
                anchors.fill: parent
                topInset: comp.topInset
                bottomInset: comp.bottomInset
                onEmptyClicked: comp.deactivateAll()
            }

            Item {
                id: workspace
                anchors.fill: parent
            }

            Rectangle {
                id: snapPreview
                visible: opacity > 0
                opacity: 0
                z: 90
                radius: 18
                color: Qt.alpha(Theme.primary, 0.22)
                border.width: 2
                border.color: Theme.primary
                Behavior on opacity { NumberAnimation { duration: 140 } }
                Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                Behavior on y { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            }

            Dock {
                shell: comp
                launcherOpen: launcher.open
                anchors.horizontalCenter: parent.horizontalCenter
                y: 12
                z: 100
                onLauncherToggled: launcher.open = !launcher.open
            }

            StatusPill {
                shell: comp
                anchors.right: parent.right
                anchors.rightMargin: 12
                y: 12
                z: 100
                onSettingsRequested: {
                    launcher.open = false
                    settingsApp.open = true
                }
            }

            Taskbar {
                shell: comp
                windows: comp.windows
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                z: 100
                onLockRequested: comp.locked = true
            }

            Launcher {
                id: launcher
                z: 200
            }

            SettingsApp {
                id: settingsApp
                z: 200
            }

            LockScreen {
                locked: comp.locked
                onUnlocked: comp.locked = false
            }

            Shortcut {
                sequence: "Ctrl+Alt+L"
                onActivated: comp.locked = !comp.locked
            }

            Shortcut {
                sequence: "Escape"
                enabled: (launcher.open || settingsApp.open) && !comp.locked
                onActivated: {
                    if (settingsApp.open)
                        settingsApp.open = false
                    else
                        launcher.open = false
                }
            }

            Shortcut {
                sequence: "Meta+A"
                enabled: !comp.locked
                onActivated: {
                    settingsApp.open = false
                    launcher.open = !launcher.open
                }
            }

            Shortcut {
                sequence: "Meta+S"
                enabled: !comp.locked
                onActivated: {
                    launcher.open = false
                    settingsApp.open = !settingsApp.open
                }
            }

            Shortcut {
                sequence: "Meta+D"
                enabled: !comp.locked
                onActivated: comp.toggleShowDesktop()
            }

            Shortcut {
                sequence: "Alt+F4"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.toplevel.sendClose()
                }
            }

            Shortcut {
                sequence: "Meta+F"
                enabled: !comp.locked
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
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("max")
                }
            }

            Shortcut {
                sequence: "Meta+Down"
                enabled: !comp.locked
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
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("left")
                }
            }

            Shortcut {
                sequence: "Meta+Right"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("right")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Left"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("tl")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Right"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("tr")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Up"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("bl")
                }
            }

            Shortcut {
                sequence: "Meta+Alt+Down"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        w.snapTo("br")
                }
            }

            Shortcut {
                sequence: "Meta+1"
                enabled: !comp.locked
                onActivated: comp.switchWorkspace(0)
            }
            Shortcut {
                sequence: "Meta+2"
                enabled: !comp.locked && comp.workspaceCount > 1
                onActivated: comp.switchWorkspace(1)
            }
            Shortcut {
                sequence: "Meta+3"
                enabled: !comp.locked && comp.workspaceCount > 2
                onActivated: comp.switchWorkspace(2)
            }
            Shortcut {
                sequence: "Meta+4"
                enabled: !comp.locked && comp.workspaceCount > 3
                onActivated: comp.switchWorkspace(3)
            }
            Shortcut {
                sequence: "Meta+5"
                enabled: !comp.locked && comp.workspaceCount > 4
                onActivated: comp.switchWorkspace(4)
            }
            Shortcut {
                sequence: "Meta+6"
                enabled: !comp.locked && comp.workspaceCount > 5
                onActivated: comp.switchWorkspace(5)
            }

            Shortcut {
                sequence: "Meta+Shift+1"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 0)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+2"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 1)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+3"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 2)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+4"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 3)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+5"
                enabled: !comp.locked
                onActivated: {
                    const w = comp.activeWindow()
                    if (w)
                        comp.sendToWorkspace(w, 4)
                }
            }
            Shortcut {
                sequence: "Meta+Shift+6"
                enabled: !comp.locked
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

    XdgDecorationManagerV1 {
        preferredMode: XdgToplevel.ServerSideDecoration
    }

    Component {
        id: chromeComponent
        Chrome {}
    }
}
