import QtQuick
import Loali

Item {
    id: desktop

    signal emptyClicked()

    property real topInset: 0
    property real bottomInset: 0

    property bool dragActive: false
    property int dragIndex: -1
    property string dragIcon: ""
    property int dropIndex: -1

    function updateDesktopDrag(vx, vy) {
        ghost.x = vx - 32
        ghost.y = grid.contentY + vy - 32
        const cx = vx
        const cy = grid.contentY + vy
        const idx = grid.indexAt(cx, cy)
        dropIndex = (idx >= 0 && idx !== dragIndex) ? idx : -1
    }

    function finishDesktopDrag(vx, vy) {
        dragActive = false
        const cx = vx
        const cy = grid.contentY + vy
        const idx = grid.indexAt(cx, cy)
        const src = grid.itemAtIndex(dragIndex)
        const dst = grid.itemAtIndex(idx)
        if (src && dst && idx >= 0 && idx !== dragIndex) {
            if (dst.isFolder)
                Apps.addToFolder(dst.folderId, src.appId)
            else if (!dst.isFolder && src.appId !== "")
                Apps.createFolder(src.appId, dst.appId)
        }
        dragIndex = -1
        dropIndex = -1
    }

    Wallpaper {
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            grid.currentIndex = -1
            folderPopup.closePopup()
            desktop.emptyClicked()
        }
    }

    DesktopModel {
        id: dmodel
    }

    GridView {
        id: grid
        x: 20
        y: desktop.topInset + 8
        width: Math.min(parent.width - 40, 6 * cellWidth)
        height: parent.height - desktop.topInset - desktop.bottomInset - 16
        cellWidth: 100
        cellHeight: 110
        model: dmodel
        clip: true

        delegate: Item {
            id: cell
            required property int index
            required property string name
            required property string icon
            required property string appId
            required property string folderId
            required property bool isFolder
            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                anchors.fill: parent
                radius: 18
                color: cell.GridView.isCurrentItem ? Qt.alpha(Theme.textOnSurface, 0.14)
                     : hover.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.07)
                     : desktop.dropIndex === cell.index && desktop.dragActive ? Qt.alpha(Theme.primary, 0.18)
                     : "transparent"
                border.width: desktop.dropIndex === cell.index && desktop.dragActive ? 2
                            : cell.GridView.isCurrentItem ? 1 : 0
                border.color: Theme.primary
                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.width { NumberAnimation { duration: 120 } }
            }

            Image {
                visible: !cell.isFolder
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 14
                width: 54
                height: 54
                source: "image://loali/" + cell.icon
                sourceSize: Qt.size(108, 108)
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }

            Rectangle {
                visible: cell.isFolder
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 14
                width: 54
                height: 54
                radius: 14
                color: Theme.surfaceContainerHigh
                border.width: 1
                border.color: Qt.alpha(Theme.outlineVariant, 0.7)

                MIcon {
                    anchors.fill: parent
                    anchors.margins: 10
                    path: Theme.iconFolder
                    color: Theme.primary
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                width: parent.width - 10
                text: cell.name
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: "#FFFFFF"
                font.pixelSize: 12
                styleColor: "#99000000"
                style: Text.Raised
            }

            MouseArea {
                id: hover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                property point pressPos
                property bool moved: false
                onPressed: (mouse) => {
                    if (mouse.button !== Qt.LeftButton)
                        return
                    moved = false
                    pressPos = Qt.point(mouse.x, mouse.y)
                }
                onPositionChanged: (mouse) => {
                    if (!pressed || !(pressedButtons & Qt.LeftButton) || cell.isFolder)
                        return
                    const dx = mouse.x - pressPos.x
                    const dy = mouse.y - pressPos.y
                    if (!desktop.dragActive && dx * dx + dy * dy > 100) {
                        moved = true
                        desktop.dragIndex = cell.index
                        desktop.dragIcon = cell.icon
                        desktop.dragActive = true
                    }
                    if (desktop.dragActive)
                        desktop.updateDesktopDrag(cell.x + mouse.x, cell.y + mouse.y - grid.contentY)
                }
                onReleased: (mouse) => {
                    if (desktop.dragActive) {
                        desktop.finishDesktopDrag(cell.x + mouse.x, cell.y + mouse.y - grid.contentY)
                        return
                    }
                    if (moved)
                        return
                    if (mouse.button === Qt.RightButton && !cell.isFolder) {
                        grid.currentIndex = cell.index
                        return
                    }
                    if (cell.isFolder) {
                        folderPopup.openFor(cell.folderId, cell)
                    } else {
                        grid.currentIndex = cell.index
                    }
                }
                onDoubleClicked: {
                    if (!cell.isFolder)
                        Apps.launchById(cell.appId)
                }
            }
        }
    }

    Rectangle {
        id: ghost
        parent: grid.contentItem
        visible: desktop.dragActive
        z: 999
        width: 64
        height: 64
        radius: 16
        color: Theme.surfaceContainerHigh
        border.width: 1
        border.color: Qt.alpha(Theme.primary, 0.6)
        opacity: 0.96
        Image {
            anchors.centerIn: parent
            width: 44
            height: 44
            source: "image://loali/" + desktop.dragIcon
            sourceSize: Qt.size(88, 88)
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
    }

    Rectangle {
        id: folderPopup
        property string folderId: ""
        property string folderName: ""
        property var entries: []
        property Item opener: null

        visible: folderId !== ""
        z: 60
        width: 288
        height: 52 + Math.min(fgrid.contentHeight, 300) + 64
        x: {
            if (!opener)
                return Math.round((desktop.width - width) / 2)
            const p = opener.mapToItem(desktop, 0, 0)
            let px = p.x + opener.width + 8
            if (px + width > desktop.width - 12)
                px = p.x - width - 8
            return Math.max(12, Math.min(px, desktop.width - width - 12))
        }
        y: {
            if (!opener)
                return Math.round((desktop.height - height) / 2)
            const p = opener.mapToItem(desktop, 0, 0)
            return Math.max(12, Math.min(p.y - 6, desktop.height - height - 12))
        }
        radius: 24
        color: Theme.surfaceContainerHigh
        border.width: 1
        border.color: Qt.alpha(Theme.outlineVariant, 0.6)
        opacity: visible ? 1 : 0
        scale: visible ? 1 : 0.92
        Behavior on opacity { NumberAnimation { duration: 140 } }
        Behavior on scale {
            NumberAnimation {
                duration: 180
                easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
            }
        }

        function openFor(fid, cellItem) {
            folderId = fid
            opener = cellItem
            reload()
        }

        function closePopup() {
            folderId = ""
            opener = null
            entries = []
        }

        function reload() {
            if (folderId === "")
                return
            entries = Apps.folderEntries(folderId)
            folderName = Apps.folderName(folderId)
            if (!Apps.hasFolder(folderId))
                closePopup()
        }

        Connections {
            target: Apps
            function onFoldersChanged() {
                folderPopup.reload()
            }
        }

        Column {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            Row {
                width: parent.width
                height: 32
                spacing: 8

                MIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    path: Theme.iconFolder
                    color: Theme.primary
                }

                TextInput {
                    id: nameField
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 36
                    text: folderPopup.folderName
                    color: Theme.textOnSurface
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    selectByMouse: true
                    onEditingFinished: {
                        if (folderPopup.folderId !== "" && text !== folderPopup.folderName && text.trim() !== "")
                            Apps.renameFolder(folderPopup.folderId, text)
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Qt.alpha(Theme.outlineVariant, 0.5)
            }

            GridView {
                id: fgrid
                width: parent.width
                height: Math.min(contentHeight, 300)
                clip: true
                cellWidth: 84
                cellHeight: 88
                model: folderPopup.entries

                delegate: Item {
                    width: fgrid.cellWidth
                    height: fgrid.cellHeight
                    required property var modelData

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 14
                        color: fhover.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.08) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 8
                        width: 40
                        height: 40
                        source: "image://loali/" + (parent.modelData ? parent.modelData.icon : "")
                        sourceSize: Qt.size(80, 80)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        width: parent.width - 12
                        text: parent.modelData ? parent.modelData.name : ""
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        color: Theme.textOnSurface
                        font.pixelSize: 11
                    }

                    MouseArea {
                        id: fhover
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                if (parent.modelData)
                                    Apps.removeFromFolder(folderPopup.folderId, parent.modelData.appId)
                                return
                            }
                            if (parent.modelData) {
                                Apps.launchById(parent.modelData.appId)
                                folderPopup.closePopup()
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: fgrid.count === 0
                    text: qsTr("Folder is empty")
                    color: Theme.onSurfaceVariant
                    font.pixelSize: 12
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    width: ugroupLbl.implicitWidth + 24
                    height: 30
                    radius: 15
                    color: Theme.surfaceContainerHighest
                    border.width: 1
                    border.color: Qt.alpha(Theme.outlineVariant, 0.6)

                    Text {
                        id: ugroupLbl
                        anchors.centerIn: parent
                        text: qsTr("Ungroup all")
                        color: Theme.textOnSurface
                        font.pixelSize: 12
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            Apps.destroyFolder(folderPopup.folderId)
                            folderPopup.closePopup()
                        }
                    }
                }
            }
        }
    }
}
