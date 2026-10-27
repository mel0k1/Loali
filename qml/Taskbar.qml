import QtQuick
import QtQuick.Controls
import QtWayland.Compositor
import Loali

Rectangle {
    id: taskbar

    signal lockRequested()

    property var shell
    property var windows

    property var previewWin: null
    property real previewX: 0

    readonly property bool fullMode: Settings.taskbarStyle === Settings.Full
    readonly property bool stretched: fullMode || overflow
    readonly property int pad: 12
    readonly property real contentW: tasksPart.implicitWidth + rightPart.implicitWidth + 12 + pad * 2
    readonly property bool overflow: !fullMode && parent ? contentW > parent.width - 16 : false

    function buttonFor(w) {
        for (let i = 0; i < tasksPart.children.length; ++i) {
            const it = tasksPart.children[i]
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
            const cx = btn.mapToItem(parent, btn.width / 2, 0).x
            previewX = Math.max(8, Math.min(parent.width - pw - 8, cx - pw / 2))
        } else if (parent) {
            previewX = Math.round((parent.width - pw) / 2)
        }
        previewWin = w
    }

    function hidePreview() {
        previewWin = null
    }

    x: stretched ? 0 : Math.round((parent.width - width) / 2)
    y: stretched ? parent.height - height : parent.height - height - 12
    width: stretched ? parent.width : Math.min(parent.width - 16, contentW)
    height: Theme.sz(50)
    radius: stretched ? 0 : height / 2
    color: Theme.surfaceContainer
    border.width: stretched ? 0 : 1
    border.color: Qt.alpha(Theme.outlineVariant, 0.6)
    clip: true

    Behavior on x {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
    }
    Behavior on y {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
    }
    Behavior on width {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
    }
    Behavior on radius {
        NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } }
    }

    Shadow {
        extent: 10
        radius: 32
        visible: !taskbar.stretched
    }

    Rectangle {
        visible: taskbar.stretched
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Qt.alpha(Theme.outlineVariant, 0.6)
    }

    Row {
        id: barRow
        anchors.verticalCenter: parent.verticalCenter
        x: taskbar.stretched ? taskbar.pad : Math.round((taskbar.width - width) / 2)
        spacing: 10

        Behavior on x {
            NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialDefault } }
        }

        Row {
            id: tasksPart
            spacing: 6

            Repeater {
                model: taskbar.windows

                TaskButton {
                    required property var win
                    winRef: win
                }
            }
        }

        Item {
            width: taskbar.stretched
                ? Math.max(10, taskbar.width - taskbar.pad * 2 - tasksPart.width - rightPart.width)
                : 12
            height: 1
        }

        Row {
            id: rightPart
            spacing: 10

            Rectangle {
                width: 1
                height: Theme.sz(24)
                anchors.verticalCenter: parent.verticalCenter
                color: Qt.alpha(Theme.outlineVariant, 0.5)
            }

            Item {
                width: Theme.sz(34)
                height: Theme.sz(34)
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.centerIn: parent
                    width: Theme.sz(30)
                    height: Theme.sz(30)
                    radius: width / 2
                    color: lockMa.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.1) : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                MIcon {
                    anchors.centerIn: parent
                    width: Theme.sz(18)
                    height: Theme.sz(18)
                    path: Theme.iconLock
                    color: Theme.onSurfaceVariant
                }

                MouseArea {
                    id: lockMa
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: taskbar.lockRequested()
                }

                ToolTip.visible: lockMa.containsMouse
                ToolTip.text: qsTr("Lock")
                ToolTip.delay: 400
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
        }
    }

    Timer {
        id: previewHide
        interval: 320
        onTriggered: taskbar.hidePreview()
    }

    Connections {
        target: taskbar.windows
        function onCountChanged() {
            if (!taskbar.previewWin)
                return
            let found = false
            for (let i = 0; i < taskbar.windows.count; ++i) {
                if (taskbar.windows.get(i).win === taskbar.previewWin) {
                    found = true
                    break
                }
            }
            if (!found)
                taskbar.hidePreview()
        }
    }

    Rectangle {
        id: preview
        parent: taskbar.parent
        visible: opacity > 0
        opacity: taskbar.previewWin !== null ? 1 : 0
        scale: opacity > 0.5 ? 1 : 0.94
        z: 120
        width: 264
        height: 218
        x: taskbar.previewX
        y: taskbar.y - height - 10
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
                id: previewIcon
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                source: "image://loali/" + (taskbar.previewWin
                    ? Apps.iconForAppId(taskbar.previewWin.toplevel.appId) : "")
                sourceSize: Qt.size(36, 36)
                asynchronous: true
            }

            Text {
                anchors.left: previewIcon.right
                anchors.leftMargin: 8
                anchors.right: previewClose.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: taskbar.previewWin ? taskbar.previewWin.toplevel.title : ""
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
                        if (taskbar.previewWin)
                            taskbar.previewWin.toplevel.sendClose()
                        taskbar.hidePreview()
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
                active: taskbar.previewWin !== null
                sourceComponent: previewSurfaceComp
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (taskbar.previewWin) {
                        taskbar.shell.restore(taskbar.previewWin)
                        taskbar.hidePreview()
                    }
                }
            }
        }
    }

    Component {
        id: previewSurfaceComp

        WaylandQuickItem {
            id: pv
            anchors.centerIn: parent
            readonly property real sw: taskbar.previewWin ? Math.max(taskbar.previewWin.width, 1) : 1
            readonly property real sh: taskbar.previewWin
                ? Math.max(taskbar.previewWin.height - taskbar.previewWin.barHeight, 1) : 1
            readonly property real fit: Math.min(shotArea.width / sw, shotArea.height / sh)
            width: sw * fit
            height: sh * fit
            surface: taskbar.previewWin && taskbar.previewWin.surface
                     ? taskbar.previewWin.surface : null
            output: taskbar.shell ? taskbar.shell.defaultOutput : null
            enabled: false
            focusOnClick: false
        }
    }

    component TaskButton: Rectangle {
        id: taskBtn
        property var winRef
        readonly property bool isActive: winRef ? (winRef.active && !winRef.minimized) : false
        width: Math.min(150, label.width + 40)
        height: Theme.sz(36)
        radius: height / 2
        color: isActive ? Theme.secondaryContainer
             : taskMa.containsMouse ? Theme.surfaceContainerHighest
             : "transparent"
        opacity: winRef && winRef.minimized ? 0.5 : 1
        anchors.verticalCenter: parent.verticalCenter
        Behavior on color { ColorAnimation { duration: 150 } }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: 10
            spacing: 7

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                source: "image://loali/" + Apps.iconForAppId(taskBtn.winRef ? taskBtn.winRef.toplevel.appId : "")
                sourceSize: Qt.size(36, 36)
                asynchronous: true
            }

            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, 96)
                text: taskBtn.winRef ? taskBtn.winRef.toplevel.title : ""
                elide: Text.ElideRight
                font.pixelSize: 12
                color: Theme.textOnSurface
            }
        }

        MouseArea {
            id: taskMa
            anchors.fill: parent
            hoverEnabled: true
            onContainsMouseChanged: {
                if (containsMouse) {
                    previewHide.stop()
                    taskbar.showPreviewFor(taskBtn.winRef)
                } else if (taskbar.previewWin === taskBtn.winRef) {
                    previewHide.restart()
                }
            }
            onClicked: {
                const w = taskBtn.winRef
                if (w.active && !w.minimized)
                    taskbar.shell.minimize(w)
                else
                    taskbar.shell.restore(w)
                taskbar.hidePreview()
            }
        }
    }
}
