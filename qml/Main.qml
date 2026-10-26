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
    property ListModel windows: ListModel {}

    function maxArea() {
        return Qt.rect(10, topInset, win.width - 20, win.height - topInset - bottomInset)
    }

    function fullArea() {
        return Qt.rect(0, 0, win.width, win.height)
    }

    function activate(c) {
        for (let i = 0; i < windows.count; ++i) {
            const w = windows.get(i).win
            w.active = (w === c)
        }
        if (!c.minimized)
            c.z = ++zTop
        defaultSeat.setKeyboardFocus(c.content.surface)
    }

    function deactivateAll() {
        for (let i = 0; i < windows.count; ++i)
            windows.get(i).win.active = false
    }

    function minimize(c) {
        c.minimized = true
        if (c.active) {
            c.active = false
            defaultSeat.setKeyboardFocus(null)
        }
    }

    function restore(c) {
        c.minimized = false
        activate(c)
    }

    function removeWindow(c) {
        for (let i = 0; i < windows.count; ++i) {
            if (windows.get(i).win === c) {
                windows.remove(i)
                break
            }
        }
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

            Dock {
                shell: comp
                launcherOpen: launcher.open
                anchors.horizontalCenter: parent.horizontalCenter
                y: 12
                z: 100
                onLauncherToggled: launcher.open = !launcher.open
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
                enabled: launcher.open && !comp.locked
                onActivated: launcher.open = false
            }
        }
    }

    XdgShell {
        onToplevelCreated: (toplevel, xdgSurface) => {
            const c = chromeComponent.createObject(workspace, {
                shellSurface: xdgSurface,
                toplevel: toplevel,
                compositor: comp
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
