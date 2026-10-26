import QtQuick
import QtQuick.Controls
import Loali

Item {
    id: lock

    property bool locked: false
    signal unlocked()

    anchors.fill: parent
    opacity: locked ? 1 : 0
    visible: opacity > 0
    enabled: locked
    Behavior on opacity { NumberAnimation { duration: 250 } }
    transform: Translate {
        y: lock.locked ? 0 : -40
        Behavior on y { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
    }

    Wallpaper {
        anchors.fill: parent
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: 0.35
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.28
        spacing: 8

        Text {
            id: bigTime
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDateTime(new Date(), "HH:mm")
            font.pixelSize: 96
            font.weight: Font.Medium
            color: Theme.onSurface
        }

        Text {
            id: bigDate
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "dddd, d MMMM")
            font.pixelSize: 20
            color: Theme.onSurfaceVariant
        }

        Timer {
            interval: 5000
            running: true
            repeat: true
            onTriggered: {
                bigTime.text = Qt.formatDateTime(new Date(), "HH:mm")
                bigDate.text = Qt.formatDate(new Date(), "dddd, d MMMM")
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: lock.unlocked()
    }

    Shortcut {
        sequence: "Return"
        enabled: lock.locked
        onActivated: lock.unlocked()
    }
}
