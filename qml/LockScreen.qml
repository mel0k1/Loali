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
    Behavior on opacity { NumberAnimation { duration: Theme.durDefault; easing { type: Easing.BezierSpline; bezierCurve: Theme.effectsDefault } } }
    transform: Translate {
        y: lock.locked ? 0 : -40
        Behavior on y { NumberAnimation { duration: Theme.durSlow; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialSlow } } }
    }

    Wallpaper {
        anchors.fill: parent
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: 0.35
    }

    MorphShape {
        id: lockBlob
        width: 260
        height: 260
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.28 - 170
        opacity: 0.28
        fromShape: lockCycle[lockStage]
        toShape: lockCycle[lockStage + 1]
        progress: lockMorph.progress
        color: Theme.primary

        readonly property var lockCycle: ["cookie9", "sunny", "clover8", "flower", "boom", "cookie9"]
        property int lockStage: 0

        SequentialAnimation on scale {
            running: lock.locked
            loops: Animation.Infinite
            NumberAnimation { to: 1.08; duration: 2600; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 2600; easing.type: Easing.InOutSine }
        }

        Timer {
            interval: 1600
            running: lock.locked
            repeat: true
            onTriggered: {
                if (lockMorph.running)
                    return
                lockBlob.lockStage = (lockBlob.lockStage + 1) % (lockBlob.lockCycle.length - 1)
                lockMorph.restart()
            }
        }

        SequentialAnimation {
            id: lockMorph
            property real progress: 0
            NumberAnimation {
                target: lockMorph
                property: "progress"
                from: 0
                to: 1
                duration: 1400
                easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialSlow }
            }
        }
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
            color: "#FFFFFF"
            styleColor: "#66000000"
            style: Text.Raised
        }

        Text {
            id: bigDate
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "dddd, d MMMM")
            font.pixelSize: 20
            color: "#FFFFFF"
            styleColor: "#66000000"
            style: Text.Raised
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
