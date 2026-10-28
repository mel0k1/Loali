import QtQuick
import QtWayland.Compositor
import Loali

Rectangle {
    id: chrome

    signal windowClosed()

    property var shellSurface
    property var toplevel: shellSurface
    property var compositor
    readonly property alias surface: content.surface
    property bool active: false
    property bool minimized: false
    property bool maximized: false
    property bool fullscreen: false
    property bool resizing: false
    property bool instant: false
    property int ws: 0
    property string tiled: ""
    property rect prevRect
    readonly property int barHeight: shellSurface && shellSurface.decorated && !fullscreen ? 40 : 0

    function toggleMaximize() {
        if (fullscreen)
            exitFullscreen()
        if (!maximized) {
            tiled = ""
            prevRect = Qt.rect(x, y, width, height)
            maximized = true
            const a = compositor.maxArea()
            applyFrame(a.x, a.y, a.width, a.height)
        } else {
            maximized = false
            applyFrame(prevRect.x, prevRect.y, prevRect.width, prevRect.height)
        }
    }

    function reapplyLayout() {
        if (!shellSurface || fullscreen || minimized)
            return
        if (maximized) {
            const a = compositor.maxArea()
            applyFrame(a.x, a.y, a.width, a.height)
        } else if (tiled !== "") {
            snapTo(tiled)
        }
    }

    function snapTo(mode) {
        if (fullscreen)
            exitFullscreen()
        if (mode === "max") {
            if (!maximized)
                toggleMaximize()
            return
        }
        if (tiled === "" && !maximized)
            prevRect = Qt.rect(x, y, width, height)
        maximized = false
        tiled = mode
        const a = compositor.maxArea()
        let nx = a.x
        let ny = a.y
        let nw = a.width / 2
        let nh = a.height
        if (mode === "right") {
            nx = a.x + a.width / 2
        } else if (mode === "tl") {
            nh = a.height / 2
        } else if (mode === "tr") {
            nx = a.x + a.width / 2
            nh = a.height / 2
        } else if (mode === "bl") {
            ny = a.y + a.height / 2
            nh = a.height / 2
        } else if (mode === "br") {
            nx = a.x + a.width / 2
            ny = a.y + a.height / 2
            nh = a.height / 2
        }
        applyFrame(nx, ny, nw, nh)
    }

    function unsnap() {
        if (tiled === "")
            return
        tiled = ""
        applyFrame(prevRect.x, prevRect.y, prevRect.width, prevRect.height)
    }

    function applyFrame(xp, yp, wp, hp) {
        x = xp
        y = yp
        width = wp
        height = hp
        if (shellSurface && shellSurface.windowId !== 0)
            compositor.xwayland.configureWindow(shellSurface.windowId,
                                                Qt.rect(xp, yp + barHeight, wp, hp - barHeight))
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

    opacity: appearing ? 0 : 1
    scale: appearing ? 0.94 : 1
    visible: !minimized && shellSurface && shellSurface.mapped
             && (compositor.expoActive || ws === compositor.currentWorkspace)
    enabled: !compositor.expoActive
    radius: maximized || fullscreen ? 0 : 18
    color: Theme.surfaceContainerLow
    border.width: 1
    border.color: fullscreen ? "transparent"
                            : (active ? Theme.primary : Theme.outlineVariant)
    clip: true

    Behavior on opacity {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } }
    }
    Behavior on scale {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
    }

    property bool appearing: true
    Component.onCompleted: {
        width = Qt.binding(() => shellSurface ? shellSurface.frame.width : 320)
        height = Qt.binding(() => shellSurface ? shellSurface.frame.height + barHeight : 160 + barHeight)
        x = Qt.binding(() => shellSurface ? shellSurface.frame.x : 0)
        y = Qt.binding(() => shellSurface ? shellSurface.frame.y - barHeight : 0)
        Qt.callLater(() => { appearing = false })
    }

    Connections {
        target: chrome.shellSurface
        function onWindowClosed() {
            chrome.windowClosed()
        }
    }

    component XBtn: Item {
        id: btn
        property string glyph
        property bool danger: false
        signal activated()
        width: 40
        height: chrome.barHeight
        Rectangle {
            anchors.centerIn: parent
            width: 30
            height: 30
            radius: 15
            color: ma.containsMouse ? (btn.danger ? Theme.error : Theme.surfaceContainerHighest) : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        MIcon {
            anchors.centerIn: parent
            width: 16
            height: 16
            path: btn.glyph
            color: ma.containsMouse && btn.danger ? "#FFFFFF" : Theme.textOnSurface
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            onClicked: btn.activated()
        }
    }

    component XResizeEdge: MouseArea {
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
            const ns = chrome.shellSurface.sizeForResize(
                Qt.size(startRect.width, startRect.height - chrome.barHeight),
                Qt.point(dx, dy), edges)
            chrome.width = Math.max(280, ns.width)
            chrome.height = Math.max(180, ns.height) + chrome.barHeight
            if (edges & Qt.LeftEdge)
                chrome.x = startRect.x + startRect.width - chrome.width
            if (edges & Qt.TopEdge)
                chrome.y = startRect.y + startRect.height - chrome.height
        }
        onReleased: {
            chrome.applyFrame(chrome.x, chrome.y, chrome.width, chrome.height)
            chrome.resizing = false
        }
    }

    Rectangle {
        id: bar
        visible: chrome.barHeight > 0
        x: 0
        y: 0
        width: parent.width
        height: 40
        color: chrome.active ? Theme.surfaceContainerHigh : Theme.surfaceContainer

        Text {
            x: 14
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - windowButtons.width - 28
            text: chrome.shellSurface ? chrome.shellSurface.title : ""
            elide: Text.ElideRight
            color: Theme.textOnSurface
            font.pixelSize: 13
        }

        Row {
            id: windowButtons
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            XBtn {
                glyph: Theme.iconMinimize
                onActivated: chrome.compositor.minimize(chrome)
            }
            XBtn {
                glyph: chrome.maximized ? Theme.iconRestore : Theme.iconMaximize
                onActivated: chrome.toggleMaximize()
            }
            XBtn {
                glyph: Theme.iconClose
                danger: true
                onActivated: chrome.shellSurface.sendClose()
            }
        }

        DragHandler {
            id: dragHandler
            enabled: !chrome.maximized && !chrome.fullscreen
            target: null
            onActiveChanged: {
                if (active) {
                    chrome.instant = true
                    if (chrome.tiled !== "") {
                        chrome.tiled = ""
                    }
                    chrome.dragStart = Qt.point(centroid.scenePosition.x - chrome.x,
                                                centroid.scenePosition.y - chrome.y)
                } else {
                    chrome.instant = false
                }
            }
            onTranslationChanged: {
                if (dragHandler.active) {
                    const nx = centroid.scenePosition.x - chrome.dragStart.x
                    const ny = centroid.scenePosition.y - chrome.dragStart.y
                    chrome.x = nx
                    chrome.y = ny
                    chrome.applyFrame(nx, ny, chrome.width, chrome.height)
                }
            }
        }

        TapHandler {
            onDoubleTapped: chrome.toggleMaximize()
        }
    }

    property point dragStart: Qt.point(0, 0)

    ShellSurfaceItem {
        id: content
        x: 0
        y: chrome.barHeight
        width: chrome.width
        height: chrome.height - chrome.barHeight
        shellSurface: chrome.shellSurface
        onSurfaceDestroyed: chrome.windowClosed()
    }


    TapHandler {
        onTapped: {
            chrome.compositor.activate(chrome)
            if (chrome.shellSurface && chrome.shellSurface.windowId !== 0)
                chrome.compositor.xwayland.focusWindow(chrome.shellSurface.windowId)
        }
    }

    XResizeEdge { edges: Qt.LeftEdge; x: 0; y: 10; width: 5; height: chrome.height - 20 }
    XResizeEdge { edges: Qt.RightEdge; x: chrome.width - 5; y: 10; width: 5; height: chrome.height - 20 }
    XResizeEdge { edges: Qt.TopEdge; x: 10; y: 0; width: chrome.width - 20; height: 5 }
    XResizeEdge { edges: Qt.BottomEdge; x: 10; y: chrome.height - 5; width: chrome.width - 20; height: 5 }
    XResizeEdge { edges: Qt.LeftEdge | Qt.TopEdge; x: 0; y: 0; width: 12; height: 12 }
    XResizeEdge { edges: Qt.RightEdge | Qt.TopEdge; x: chrome.width - 12; y: 0; width: 12; height: 12 }
    XResizeEdge { edges: Qt.LeftEdge | Qt.BottomEdge; x: 0; y: chrome.height - 12; width: 12; height: 12 }
    XResizeEdge { edges: Qt.RightEdge | Qt.BottomEdge; x: chrome.width - 12; y: chrome.height - 12; width: 12; height: 12 }
}
