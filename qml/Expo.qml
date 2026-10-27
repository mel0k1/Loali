import QtQuick
import Loali

Item {
    id: expo

    property var shell
    property var winLayer
    property bool active: false
    property int frozen: 4
    property var dragWin: null
    property int dragCell: -1
    property real grabX: 0
    property real grabY: 0
    property point pressPos
    property bool moved: false

    readonly property real viewW: width
    readonly property real viewH: height
    readonly property real cellW: {
        const n = frozen
        if (n <= 0 || width <= 0)
            return 260
        const gap = 20
        return Math.max(Math.min((width - gap * (n + 1)) / n, 460), 170)
    }

    anchors.fill: parent
    opacity: active ? 1 : 0
    visible: opacity > 0
    enabled: active
    Behavior on opacity { NumberAnimation { duration: 180 } }

    function holderOf(i) {
        if (i < 0 || i >= cellsRep.count)
            return null
        const c = cellsRep.itemAt(i)
        return c ? c.holder : null
    }

    function winsFor(i) {
        const res = []
        if (!shell)
            return res
        for (let k = 0; k < shell.windows.count; ++k) {
            const w = shell.windows.get(k).win
            if (w.ws === i && !w.minimized)
                res.push(w)
        }
        return res
    }

    function cellAt(x, y) {
        for (let i = 0; i < cellsRep.count; ++i) {
            const c = cellsRep.itemAt(i)
            if (!c)
                continue
            const p = expo.mapToItem(c, x, y)
            if (p.x >= 0 && p.x <= c.width && p.y >= 0 && p.y <= c.height)
                return i
        }
        return -1
    }

    function pickWindow(x, y) {
        if (!shell)
            return null
        let best = null
        for (let k = 0; k < shell.windows.count; ++k) {
            const w = shell.windows.get(k).win
            if (w.minimized)
                continue
            const h = holderOf(w.ws)
            if (!h)
                continue
            const p = expo.mapToItem(h, x, y)
            if (p.x >= w.x && p.x <= w.x + w.width && p.y >= w.y && p.y <= w.y + w.height) {
                if (!best || w.z > best.z)
                    best = w
            }
        }
        return best
    }

    function open() {
        if (active || !shell || !winLayer)
            return
        frozen = shell.workspaceCount
        shell.expoActive = true
        active = true
        dragWin = null
        dragCell = -1
        for (let k = 0; k < shell.windows.count; ++k) {
            const w = shell.windows.get(k).win
            if (w.minimized)
                continue
            const h = holderOf(w.ws)
            if (h)
                w.parent = h
        }
    }

    function close() {
        if (!active)
            return
        dragWin = null
        dragCell = -1
        if (shell && winLayer) {
            for (let k = 0; k < shell.windows.count; ++k) {
                const w = shell.windows.get(k).win
                w.parent = winLayer
            }
        }
        if (shell)
            shell.expoActive = false
        active = false
    }

    function toggle() {
        if (active)
            close()
        else
            open()
    }

    Connections {
        target: shell
        function onWorkspaceCountChanged() {
            if (expo.active)
                expo.close()
        }
    }

    Connections {
        target: shell ? shell.windows : null
        function onCountChanged() {
            if (!expo.active || !winLayer)
                return
            for (let k = 0; k < shell.windows.count; ++k) {
                const w = shell.windows.get(k).win
                const h = holderOf(w.ws)
                if (h && w.parent !== h)
                    w.parent = h
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: 0.62
        MouseArea {
            anchors.fill: parent
            onClicked: expo.close()
        }
    }

    Row {
        id: layoutRow
        anchors.centerIn: parent
        spacing: 20

        Repeater {
            id: cellsRep
            model: expo.active ? expo.frozen : 0

            delegate: Rectangle {
                id: cell
                required property int index
                readonly property bool current: expo.shell && expo.shell.currentWorkspace === cell.index
                readonly property bool dragOver: expo.dragCell === cell.index
                readonly property int winCount: expo.winsFor(cell.index).length

                width: expo.cellW
                height: expo.cellW * expo.viewH / Math.max(expo.viewW, 1)
                radius: 20
                color: Theme.surfaceContainerLow
                border.width: cell.dragOver ? 2 : (cell.current ? 1 : 0)
                border.color: Theme.primary
                clip: true
                opacity: expo.active ? 1 : 0
                scale: expo.active ? 1 : 0.9
                Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Behavior on scale {
                    NumberAnimation {
                        duration: 280
                        easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
                    }
                }
                Behavior on border.width { NumberAnimation { duration: 120 } }

                Wallpaper {
                    anchors.fill: parent
                }

                Item {
                    id: holder
                    objectName: "expoHolder" + cell.index
                    width: expo.viewW
                    height: expo.viewH
                    scale: cell.width / Math.max(expo.viewW, 1)
                    transformOrigin: Item.TopLeft
                    enabled: false
                }

                Text {
                    anchors.centerIn: parent
                    visible: cell.winCount === 0
                    text: qsTr("No windows")
                    color: Theme.onSurfaceVariant
                    font.pixelSize: 13
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 10
                    width: chipLabel.implicitWidth + 22
                    height: 28
                    radius: 14
                    color: cell.current ? Theme.primary : Qt.alpha(Theme.surfaceContainerHigh, 0.92)
                    border.width: 1
                    border.color: cell.current ? Theme.primary : Qt.alpha(Theme.outlineVariant, 0.6)

                    Text {
                        id: chipLabel
                        anchors.centerIn: parent
                        text: qsTr("Workspace %1").arg(cell.index + 1)
                              + (cell.winCount > 0 ? " · " + cell.winCount : "")
                        color: cell.current ? Theme.onPrimary : Theme.onSurface
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!expo.shell)
                            return
                        expo.shell.switchWorkspace(cell.index)
                        expo.close()
                    }
                }
            }
        }
    }

    MouseArea {
        id: dragSurface
        anchors.fill: parent
        z: 50
        hoverEnabled: true
        cursorShape: expo.dragWin ? Qt.ClosedHandCursor : Qt.ArrowCursor
        onPressed: (mouse) => {
            const w = expo.pickWindow(mouse.x, mouse.y)
            if (!w) {
                mouse.accepted = false
                return
            }
            const h = expo.holderOf(w.ws)
            if (!h) {
                mouse.accepted = false
                return
            }
            expo.dragWin = w
            expo.moved = false
            expo.pressPos = Qt.point(mouse.x, mouse.y)
            const p = expo.mapToItem(h, mouse.x, mouse.y)
            expo.grabX = p.x - w.x
            expo.grabY = p.y - w.y
            expo.dragCell = w.ws
        }
        onPositionChanged: (mouse) => {
            const w = expo.dragWin
            if (!w)
                return
            if (!expo.moved) {
                const dx = mouse.x - expo.pressPos.x
                const dy = mouse.y - expo.pressPos.y
                if (dx * dx + dy * dy < 64)
                    return
                expo.moved = true
            }
            const h = expo.holderOf(w.ws)
            if (!h)
                return
            const p = expo.mapToItem(h, mouse.x, mouse.y)
            w.x = p.x - expo.grabX
            w.y = p.y - expo.grabY
            expo.dragCell = expo.cellAt(mouse.x, mouse.y)
        }
        onReleased: (mouse) => {
            const w = expo.dragWin
            if (!w)
                return
            const target = expo.cellAt(mouse.x, mouse.y)
            if (expo.moved && target >= 0 && target !== w.ws && expo.shell) {
                const h = expo.holderOf(target)
                if (h)
                    w.parent = h
                expo.shell.sendToWorkspace(w, target)
            } else if (!expo.moved && expo.shell) {
                expo.shell.switchWorkspace(w.ws)
                expo.close()
            }
            expo.dragWin = null
            expo.dragCell = -1
        }
    }
}
