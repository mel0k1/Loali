import QtQuick
import Loali

Item {
    id: desktop

    signal emptyClicked()

    property real topInset: 0
    property real bottomInset: 0

    Wallpaper {
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            grid.currentIndex = -1
            desktop.emptyClicked()
        }
    }

    GridView {
        id: grid
        x: 20
        y: desktop.topInset + 8
        width: Math.min(parent.width - 40, 6 * cellWidth)
        height: parent.height - desktop.topInset - desktop.bottomInset - 16
        cellWidth: 100
        cellHeight: 110
        model: Apps
        clip: true

        delegate: Item {
            id: cell
            required property int index
            required property string name
            required property string icon
            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                anchors.fill: parent
                radius: 18
                color: cell.GridView.isCurrentItem ? Qt.alpha(Theme.onSurface, 0.14)
                     : hover.containsMouse ? Qt.alpha(Theme.onSurface, 0.07)
                     : "transparent"
                border.width: 1
                border.color: cell.GridView.isCurrentItem ? Qt.alpha(Theme.primary, 0.7) : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Image {
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
                onClicked: grid.currentIndex = cell.index
                onDoubleClicked: Apps.launch(cell.index)
            }
        }
    }
}
