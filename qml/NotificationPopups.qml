import QtQuick
import Loali

Item {
    id: root

    anchors.top: parent.top
    anchors.right: parent.right
    anchors.rightMargin: 12
    anchors.topMargin: 90
    width: 372
    z: 150
    visible: !Settings.dnd && Notifications.count > 0

    ListView {
        id: stack

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: contentHeight
        interactive: false
        spacing: 10
        clip: false
        model: Notifications

        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220 }
            NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: 260;
                              easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized } }
        }

        displaced: Transition {
            NumberAnimation { property: "y"; duration: 260;
                              easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized } }
        }

        delegate: Rectangle {
            id: card

            required property int index
            required property string appName
            required property string appIcon
            required property string preview
            required property string summary
            required property string body
            required property int urgency
            required property var actions

            readonly property var actionKeys: actions ? Object.keys(actions) : []

            width: ListView.view.width
            height: content.implicitHeight + 28
            radius: 24
            color: Theme.surfaceContainerHigh
            border.width: 1
            border.color: urgency === 2 ? Theme.error : Qt.alpha(Theme.outlineVariant, 0.6)

            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

            Shadow {
                extent: 8
                radius: 24
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (card.actionKeys.length > 0)
                        Notifications.invokeAction(card.index, card.actionKeys[0])
                    else
                        Notifications.dismiss(card.index)
                }
            }

            Row {
                id: content

                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.surfaceContainerHighest

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        visible: card.preview === "" && card.appIcon !== ""
                        source: visible ? "image://loali/" + card.appIcon : ""
                        sourceSize: Qt.size(48, 48)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 34
                        height: 34
                        visible: card.preview !== ""
                        source: card.preview
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    MIcon {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        visible: card.preview === "" && card.appIcon === ""
                        path: Theme.iconBell
                        color: Theme.onSurfaceVariant
                    }
                }

                Column {
                    width: parent.width - 40 - 24 - 12 * 2
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 3

                    Text {
                        width: parent.width
                        text: card.summary
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: Theme.textOnSurface
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: card.body
                        visible: card.body !== ""
                        font.pixelSize: 13
                        color: Theme.onSurfaceVariant
                        elide: Text.ElideRight
                        maximumLineCount: 3
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        width: parent.width
                        text: card.appName
                        visible: card.appName !== ""
                        font.pixelSize: 11
                        color: Theme.onSurfaceVariant
                        opacity: 0.7
                        elide: Text.ElideRight
                    }
                }

                Item {
                    width: 28
                    height: 28
                    anchors.verticalCenter: parent.verticalCenter

                    Rectangle {
                        anchors.centerIn: parent
                        width: 28
                        height: 28
                        radius: 14
                        color: closeMa.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.12) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    MIcon {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        path: Theme.iconClose
                        color: Theme.onSurfaceVariant
                    }

                    MouseArea {
                        id: closeMa
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: Notifications.dismiss(card.index)
                    }
                }
            }
        }
    }
}
