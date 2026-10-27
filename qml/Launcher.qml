import QtQuick
import QtQuick.Controls
import Loali

Item {
    id: launcher

    property bool open: false
    property var shell: null

    anchors.fill: parent
    opacity: open ? 1 : 0
    visible: opacity > 0
    enabled: open
    Behavior on opacity { NumberAnimation { duration: 180 } }

    function runningFor(appId) {
        return shell ? shell.windowsForApp(appId).length > 0 : false
    }

    function activate(appId) {
        if (launcher.runningFor(appId) && launcher.shell) {
            const wins = launcher.shell.windowsForApp(appId)
            if (wins.length > 0) {
                launcher.shell.activate(wins[0])
                launcher.open = false
                field.text = ""
                return
            }
        }
        Apps.launchById(appId)
        launcher.open = false
        field.text = ""
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: 0.5
        MouseArea {
            anchors.fill: parent
            onClicked: launcher.open = false
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(760, launcher.width * 0.9)
        height: Math.min(640, launcher.height * 0.88)
        radius: 32
        color: Theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(Theme.outlineVariant, 0.6)
        scale: launcher.open ? 1 : 0.92
        Behavior on scale {
            NumberAnimation {
                duration: 260
                easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
            }
        }

        Shadow {
            extent: 16
            radius: 32
        }

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10

            Rectangle {
                width: parent.width
                height: 52
                radius: 26
                color: Theme.surfaceContainerHighest

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 10

                    MIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 20
                        height: 20
                        path: Theme.iconSearch
                        color: Theme.onSurfaceVariant
                    }

                    TextField {
                        id: field
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 30
                        placeholderText: qsTr("Search apps")
                        color: Theme.onSurface
                        placeholderTextColor: Theme.onSurfaceVariant
                        font.pixelSize: 15
                        background: null
                        onTextChanged: Apps.query = field.text
                        onAccepted: {
                            const id = Apps.firstMatch()
                            if (id !== "") {
                                launcher.activate(id)
                                Apps.query = ""
                            }
                        }
                    }
                }
            }

            GridView {
                id: appsGrid
                width: parent.width
                height: parent.height - 88
                clip: true
                readonly property int columns: Math.max(4, Math.floor(width / 120))
                cellWidth: Math.floor(width / columns)
                cellHeight: 116
                model: Apps
                boundsBehavior: Flickable.StopAtBounds
                readonly property bool canDrag: Apps.query === ""

                move: Transition {
                    NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic }
                }
                moveDisplaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic }
                }

                Rectangle {
                    id: dragGhost
                    parent: appsGrid.contentItem
                    visible: appsGrid.dragActive
                    z: 999
                    width: 64
                    height: 64
                    radius: 16
                    color: Theme.surfaceContainerHighest
                    border.width: 1
                    border.color: Qt.alpha(Theme.primary, 0.5)
                    opacity: 0.96
                    Image {
                        anchors.centerIn: parent
                        width: 44
                        height: 44
                        source: appsGrid.dragIcon
                        sourceSize: Qt.size(88, 88)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }
                }

                function startDrag(i, icon) {
                    dragIndex = i
                    dragIcon = "image://loali/" + icon
                    dragActive = true
                }

                function updateDrag(x, y) {
                    const rows = Math.max(1, Math.ceil(count / columns))
                    const col = Math.max(0, Math.min(columns - 1, Math.floor(x / cellWidth)))
                    const row = Math.max(0, Math.min(rows - 1, Math.floor(y / cellHeight)))
                    const target = Math.min(count - 1, row * columns + col)
                    if (target !== dragIndex && dragIndex >= 0) {
                        Apps.move(dragIndex, target)
                        dragIndex = target
                    }
                    dragGhost.x = x - 32
                    dragGhost.y = y - 32
                }

                function endDrag() {
                    dragActive = false
                    dragIndex = -1
                }

                property bool dragActive: false
                property int dragIndex: -1
                property string dragIcon: ""

                delegate: Item {
                    id: appCell
                    required property int index
                    required property string name
                    required property string appId
                    required property string icon
                    required property bool pinned
                    width: appsGrid.cellWidth
                    height: appsGrid.cellHeight
                    readonly property bool running: launcher.runningFor(appCell.appId)
                    readonly property bool dragging: appsGrid.dragActive && appsGrid.dragIndex === appCell.index
                    scale: press.pressed && !appCell.dragging ? 0.94 : 1
                    opacity: appCell.dragging ? 0.25 : 1
                    Behavior on scale {
                        NumberAnimation {
                            duration: 160
                            easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
                        }
                    }
                    Behavior on opacity { NumberAnimation { duration: 120 } }

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 20
                        color: press.containsMouse || appCell.dragging ? Qt.alpha(Theme.onSurface, 0.08) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 14
                        width: 56
                        height: 56
                        source: "image://loali/" + appCell.icon
                        sourceSize: Qt.size(112, 112)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: appCell.running ? 20 : 16
                        width: parent.width - 16
                        text: appCell.name
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        color: appCell.running ? Theme.primary : Theme.onSurface
                        font.pixelSize: 12
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                    }

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 8
                        width: 5
                        height: 5
                        radius: 2.5
                        color: Theme.primary
                        opacity: appCell.running ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }

                    Rectangle {
                        id: pinChip
                        anchors.top: parent.top
                        anchors.topMargin: 7
                        anchors.right: parent.right
                        anchors.rightMargin: 7
                        width: 20
                        height: 20
                        radius: 10
                        color: Theme.surfaceContainerHighest
                        border.width: 1
                        border.color: appCell.pinned ? Theme.primary : Qt.alpha(Theme.outlineVariant, 0.7)
                        opacity: appCell.pinned ? 1 : (press.containsMouse && appsGrid.canDrag ? 0.85 : 0)
                        scale: appCell.pinned ? 1 : 0.8
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                        Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutBack } }
                        MIcon {
                            anchors.centerIn: parent
                            width: 12
                            height: 12
                            path: Theme.iconPin
                            color: appCell.pinned ? Theme.primary : Theme.onSurfaceVariant
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Apps.toggleLauncherPin(appCell.appId)
                        }
                    }

                    MouseArea {
                        id: press
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: appCell.dragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                        property point pressPos
                        property bool moved: false
                        onPressed: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                Apps.toggleLauncherPin(appCell.appId)
                                return
                            }
                            moved = false
                            pressPos = Qt.point(mouse.x, mouse.y)
                        }
                        onPositionChanged: (mouse) => {
                            if (!press.pressed || !(press.pressedButtons & Qt.LeftButton))
                                return
                            const dx = mouse.x - pressPos.x
                            const dy = mouse.y - pressPos.y
                            if (!appsGrid.dragActive && appsGrid.canDrag && dx * dx + dy * dy > 100) {
                                moved = true
                                appsGrid.startDrag(appCell.index, appCell.icon)
                                appsGrid.updateDrag(appCell.x + mouse.x, appCell.y + mouse.y)
                            }
                            if (appsGrid.dragActive)
                                appsGrid.updateDrag(appCell.x + mouse.x, appCell.y + mouse.y)
                        }
                        onReleased: {
                            if (appsGrid.dragActive)
                                appsGrid.endDrag()
                            else if (!moved)
                                launcher.activate(appCell.appId)
                        }
                        onCanceled: {
                            if (appsGrid.dragActive)
                                appsGrid.endDrag()
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: appsGrid.count === 0
                    text: qsTr("Nothing found")
                    color: Theme.onSurfaceVariant
                    font.pixelSize: 14
                }

                ScrollIndicator {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            visible: appsGrid.count > 0 && appsGrid.canDrag
            text: qsTr("Drag to reorder, click the pin to keep apps on top")
            color: Theme.onSurfaceVariant
            opacity: 0.7
            font.pixelSize: 11
        }
    }
}
