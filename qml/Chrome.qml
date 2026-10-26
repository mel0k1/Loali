import QtQuick
import QtWayland.Compositor
import QtWayland.Compositor.XdgShell
import Loali

Rectangle {
    id: chrome

    signal windowClosed()

    property var shellSurface
    property var toplevel
    property var compositor
    property bool active: false
    property bool minimized: false
    property bool maximized: false
    property bool fullscreen: false
    property bool resizing: false
    property rect prevRect
    readonly property bool decorated: toplevel.decorationMode === XdgToplevel.ServerSideDecoration
    readonly property int barHeight: decorated ? 40 : 0

    function toggleMaximize() {
        if (!maximized) {
            prevRect = Qt.rect(x, y, width, height)
            maximized = true
            const a = compositor.maxArea()
            x = a.x
            y = a.y
            width = a.width
            height = a.height
            toplevel.sendMaximized(Qt.size(content.width, content.height))
        } else {
            maximized = false
            x = prevRect.x
            y = prevRect.y
            width = prevRect.width
            height = prevRect.height
            toplevel.sendUnmaximized()
        }
    }

    function enterFullscreen() {
        if (fullscreen)
            return
        prevRect = Qt.rect(x, y, width, height)
        fullscreen = true
        x = 0
        y = 0
        width = compositor.fullArea().width
        height = compositor.fullArea().height
        z = 400
    }

    function exitFullscreen() {
        if (!fullscreen)
            return
        fullscreen = false
        x = prevRect.x
        y = prevRect.y
        width = prevRect.width
        height = prevRect.height
    }

    visible: !minimized
    radius: maximized || fullscreen ? 0 : 18
    color: Theme.surfaceContainerLow
    border.width: 1
    border.color: fullscreen ? "transparent"
                            : (active ? Theme.primary : Theme.outlineVariant)
    clip: true

    Behavior on x {
        enabled: !resizing && !dragHandler.active
        NumberAnimation { duration: 300; easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized } }
    }
    Behavior on y {
        enabled: !resizing && !dragHandler.active
        NumberAnimation { duration: 300; easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized } }
    }
    Behavior on width {
        enabled: !resizing
        NumberAnimation { duration: 300; easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized } }
    }
    Behavior on height {
        enabled: !resizing
        NumberAnimation { duration: 300; easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized } }
    }
    Behavior on radius {
        NumberAnimation { duration: 200 }
    }

    Component.onCompleted: {
        const a = compositor.maxArea()
        const gw = Math.max(shellSurface.windowGeometry.width, 320)
        const gh = Math.max(shellSurface.windowGeometry.height, 200)
        width = gw
        height = gh + barHeight
        x = Math.max(a.x, a.x + (a.width - width) / 2 + (Math.random() * 90 - 45))
        y = Math.max(a.y, a.y + (a.height - height) / 2 + (Math.random() * 60 - 30))
    }

    Connections {
        target: chrome.shellSurface
        function onWindowGeometryChanged() {
            if (!chrome.maximized && !chrome.fullscreen && !chrome.resizing
                && chrome.shellSurface.windowGeometry.width > 0) {
                chrome.width = chrome.shellSurface.windowGeometry.width
                chrome.height = chrome.shellSurface.windowGeometry.height + chrome.barHeight
            }
        }
    }

    Connections {
        target: chrome.toplevel
        function onMaximizedChanged() {
            if (chrome.toplevel.maximized !== chrome.maximized)
                chrome.toggleMaximize()
        }
        function onFullscreenChanged() {
            if (chrome.toplevel.fullscreen && !chrome.fullscreen)
                chrome.enterFullscreen()
            else if (!chrome.toplevel.fullscreen && chrome.fullscreen)
                chrome.exitFullscreen()
        }
    }

    component WinBtn: Item {
        id: btn
        property string glyph
        property color hoverFill: Theme.surfaceContainerHighest
        property bool danger: false
        signal activated()
        width: 40
        height: chrome.barHeight
        Rectangle {
            anchors.centerIn: parent
            width: 30
            height: 30
            radius: 15
            color: ma.containsMouse ? btn.hoverFill : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        MIcon {
            anchors.centerIn: parent
            width: 16
            height: 16
            path: btn.glyph
            color: ma.containsMouse && btn.danger ? "#FFFFFF" : Theme.onSurface
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            onClicked: btn.activated()
        }
    }

    component ResizeEdge: MouseArea {
        id: edge
        property int edges: 0
        property point pressPos
        property rect startRect
        z: 60
        cursorShape: (edges & (Qt.LeftEdge | Qt.RightEdge)) && (edges & (Qt.TopEdge | Qt.BottomEdge))
                     ? (((edges & (Qt.LeftEdge | Qt.TopEdge)) === (Qt.LeftEdge | Qt.TopEdge)
                         || (edges & (Qt.RightEdge | Qt.BottomEdge)) === (Qt.RightEdge | Qt.BottomEdge))
                         ? Qt.SizeFDiagCursor : Qt.SizeBDiagCursor)
                     : ((edges & (Qt.LeftEdge | Qt.RightEdge)) ? Qt.SizeHorCursor : Qt.SizeVerCursor)
        onPressed: (mouse) => {
            pressPos = Qt.point(mouse.x, mouse.y)
            startRect = Qt.rect(chrome.x, chrome.y, chrome.width, chrome.height)
            chrome.resizing = true
        }
        onPositionChanged: (mouse) => {
            const dx = mouse.x - pressPos.x
            const dy = mouse.y - pressPos.y
            const ns = chrome.toplevel.sizeForResize(
                Qt.size(startRect.width, startRect.height - chrome.barHeight),
                Qt.point(dx, dy), edges)
            const w = Math.max(280, ns.width)
            const h = Math.max(180, ns.height)
            chrome.width = w
            chrome.height = h + chrome.barHeight
            if (edges & Qt.LeftEdge)
                chrome.x = startRect.x + startRect.width - w
            if (edges & Qt.TopEdge)
                chrome.y = startRect.y + startRect.height - h
            chrome.toplevel.sendResizing(Qt.size(chrome.width, chrome.height - chrome.barHeight))
        }
        onReleased: {
            chrome.toplevel.sendConfigure(Qt.size(chrome.width, chrome.height - chrome.barHeight), [])
            chrome.resizing = false
        }
    }

    Rectangle {
        id: bar
        visible: chrome.decorated && !chrome.fullscreen
        x: 0
        y: 0
        width: parent.width
        height: chrome.barHeight
        color: chrome.active ? Theme.surfaceContainerHigh : Theme.surfaceContainer

        Text {
            x: 14
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - windowButtons.width - 28
            text: chrome.toplevel.title
            elide: Text.ElideRight
            color: Theme.onSurface
            font.pixelSize: 13
        }

        Row {
            id: windowButtons
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            WinBtn {
                glyph: Theme.iconMinimize
                onActivated: chrome.compositor.minimize(chrome)
            }
            WinBtn {
                glyph: chrome.maximized ? Theme.iconRestore : Theme.iconMaximize
                onActivated: chrome.toggleMaximize()
            }
            WinBtn {
                glyph: Theme.iconClose
                hoverFill: Theme.error
                danger: true
                onActivated: chrome.toplevel.sendClose()
            }
        }

        DragHandler {
            id: dragHandler
            enabled: !chrome.maximized && !chrome.fullscreen
            target: chrome
        }

        TapHandler {
            onDoubleTapped: chrome.toggleMaximize()
        }
    }

    ShellSurfaceItem {
        id: content
        x: 0
        y: chrome.barHeight
        width: chrome.width
        height: chrome.height - chrome.barHeight
        shellSurface: chrome.shellSurface
        moveItem: chrome
        focusOnClicked: true
        onSurfaceDestroyed: chrome.windowClosed()
    }

    TapHandler {
        onTapped: chrome.compositor.activate(chrome)
    }

    ResizeEdge { edges: Qt.LeftEdge; x: 0; y: 10; width: 5; height: chrome.height - 20 }
    ResizeEdge { edges: Qt.RightEdge; x: chrome.width - 5; y: 10; width: 5; height: chrome.height - 20 }
    ResizeEdge { edges: Qt.TopEdge; x: 10; y: 0; width: chrome.width - 20; height: 5 }
    ResizeEdge { edges: Qt.BottomEdge; x: 10; y: chrome.height - 5; width: chrome.width - 20; height: 5 }
    ResizeEdge { edges: Qt.LeftEdge | Qt.TopEdge; x: 0; y: 0; width: 12; height: 12 }
    ResizeEdge { edges: Qt.RightEdge | Qt.TopEdge; x: chrome.width - 12; y: 0; width: 12; height: 12 }
    ResizeEdge { edges: Qt.LeftEdge | Qt.BottomEdge; x: 0; y: chrome.height - 12; width: 12; height: 12 }
    ResizeEdge { edges: Qt.RightEdge | Qt.BottomEdge; x: chrome.width - 12; y: chrome.height - 12; width: 12; height: 12 }
}
