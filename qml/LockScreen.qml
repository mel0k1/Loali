import QtQuick
import QtQuick.Controls
import Loali

Item {
    id: lock

    property bool locked: false
    signal unlocked()

    readonly property bool pinRequired: Settings.pinSet
    readonly property int pinLength: Math.max(1, Settings.pinLength)
    property string pinEntry: ""
    property int pinFailures: 0
    property bool pinLockedOut: false
    property bool pinError: false
    property real shakeX: 0

    anchors.fill: parent
    opacity: locked ? 1 : 0
    visible: opacity > 0
    enabled: locked
    onLockedChanged: {
        pinEntry = ""
        pinFailures = 0
        pinError = false
        if (locked)
            lock.forceActiveFocus()
    }

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

    Column {
        id: pinArea
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.52
        spacing: 16
        visible: lock.pinRequired
        opacity: lock.pinLockedOut ? 0.45 : 1
        transform: Translate { x: lock.shakeX }

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Row {
            id: pinDots
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 18

            Repeater {
                model: lock.pinLength

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    readonly property bool filled: index < lock.pinEntry.length
                    color: filled ? (lock.pinError ? Theme.error : Theme.primary)
                                  : Qt.alpha("#FFFFFF", 0.35)
                    border.width: filled ? 0 : 2
                    border.color: lock.pinError ? Theme.error : Qt.alpha("#FFFFFF", 0.55)

                    Behavior on color { ColorAnimation { duration: 150 } }
                    scale: filled ? 1.15 : 1.0
                    Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutBack } }
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: lock.pinLockedOut ? qsTr("Too many attempts")
                  : lock.pinError ? qsTr("Wrong PIN") : qsTr("Enter PIN")
            font.pixelSize: 13
            font.weight: Font.Medium
            color: lock.pinError ? Theme.error : "#B3FFFFFF"
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        Grid {
            id: keypad
            anchors.horizontalCenter: parent.horizontalCenter
            columns: 3
            spacing: 12
            enabled: !lock.pinLockedOut && !lock.pinError

            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9"]

                PinKey {
                    label: modelData
                    onClicked: lock.pushDigit(modelData)
                }
            }

            PinKey {
                glyph: Theme.iconBackspace
                onClicked: lock.eraseDigit()
            }

            PinKey {
                label: "0"
                onClicked: lock.pushDigit("0")
            }

            PinKey {
                accent: lock.pinEntry.length === lock.pinLength
                glyph: Theme.iconCheck
                onClicked: lock.submitPin()
            }
        }
    }

    component PinKey: Rectangle {
        id: key
        property string label: ""
        property string glyph: ""
        property bool accent: false
        signal clicked()

        width: 60
        height: 60
        radius: 30
        color: accent ? Theme.primary
                      : ma.containsMouse ? Theme.surfaceContainerHighest
                      : "#33FFFFFF"
        Behavior on color { ColorAnimation { duration: 120 } }

        scale: ma.pressed ? 0.88 : 1.0
        Behavior on scale { NumberAnimation { duration: 160; easing { type: Easing.BezierSpline; bezierCurve: Theme.spatialFast } } }

        MIcon {
            anchors.centerIn: parent
            width: 22
            height: 22
            visible: key.glyph !== ""
            path: key.glyph
            color: key.accent ? Theme.onPrimary : "#FFFFFF"
        }

        Text {
            anchors.centerIn: parent
            visible: key.glyph === ""
            text: key.label
            font.pixelSize: 22
            font.weight: Font.Medium
            color: key.accent ? Theme.onPrimary : "#FFFFFF"
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            onClicked: key.clicked()
        }
    }

    SequentialAnimation {
        id: pinShake
        running: false

        NumberAnimation { target: lock; property: "shakeX"; from: -12; to: 12; duration: 50; easing.type: Easing.InOutSine }
        NumberAnimation { target: lock; property: "shakeX"; from: 12; to: -9; duration: 50; easing.type: Easing.InOutSine }
        NumberAnimation { target: lock; property: "shakeX"; from: -9; to: 6; duration: 50; easing.type: Easing.InOutSine }
        NumberAnimation { target: lock; property: "shakeX"; from: 6; to: 0; duration: 60; easing.type: Easing.InOutSine }
    }

    SequentialAnimation {
        id: pinSuccess

        NumberAnimation { target: pinArea; property: "scale"; to: 1.06; duration: 140; easing.type: Easing.OutCubic }
        NumberAnimation { target: pinArea; property: "scale"; to: 1.0; duration: 160; easing.type: Easing.InOutCubic }
    }

    function pushDigit(d) {
        if (!pinRequired || pinLockedOut || pinError)
            return
        if (pinEntry.length >= pinLength)
            return
        pinEntry += d
        if (pinEntry.length === pinLength)
            submitPin()
    }

    function eraseDigit() {
        if (!pinRequired || pinLockedOut || pinError)
            return
        pinEntry = pinEntry.slice(0, -1)
    }

    function submitPin() {
        if (!pinRequired || pinLockedOut)
            return
        if (pinEntry.length < pinLength)
            return
        if (Settings.verifyPin(pinEntry)) {
            pinSuccess.start()
            unlocked()
        } else {
            pinError = true
            pinFailures += 1
            pinShake.start()
            if (pinFailures >= 3) {
                pinLockedOut = true
                cooldownTimer.restart()
            }
            clearTimer.restart()
        }
    }

    Timer {
        id: clearTimer
        interval: 550
        onTriggered: {
            if (lock.pinError) {
                lock.pinEntry = ""
                lock.pinError = false
            }
        }
    }

    Timer {
        id: cooldownTimer
        interval: 5000
        onTriggered: {
            lock.pinLockedOut = false
            lock.pinFailures = 0
            lock.pinEntry = ""
            lock.pinError = false
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (!lock.pinRequired)
                lock.unlocked()
        }
    }

    Keys.onPressed: (event) => {
        if (!lock.pinRequired)
            return
        if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9) {
            lock.pushDigit(String(event.key - Qt.Key_0))
            event.accepted = true
        } else if (event.key === Qt.Key_Backspace) {
            lock.eraseDigit()
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            lock.submitPin()
            event.accepted = true
        }
    }

    Shortcut {
        sequence: "Return"
        enabled: lock.locked && !lock.pinRequired
        onActivated: lock.unlocked()
    }
}
