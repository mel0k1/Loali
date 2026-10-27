import QtQuick
import Loali

Item {
    id: panel

    property bool openState: false
    property var shell
    property int anchorX: 0
    property int anchorY: 0

    anchors.fill: parent
    visible: opacity > 0
    enabled: openState
    opacity: openState ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 150 } }

    TrayMenu { id: menu }

    function openAt(row, x, y) {
        const info = Tray.itemInfo(row)
        if (!info || info.menuPath === undefined || info.menuPath === "")
            return false
        menu.bus = info.menuService !== "" ? info.menuService : info.bus
        menu.path = info.menuPath
        anchorX = x
        anchorY = y
        menu.open()
        openState = true
        return true
    }

    function close() {
        openState = false
        menu.close()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: panel.close()
        onWheel: panel.close()
    }

    Rectangle {
        id: card
        x: Math.max(12, Math.min(panel.anchorX - width / 2, panel.width - width - 12))
        y: Math.max(12, Math.min(panel.anchorY, panel.height - height - 12))
        width: 320
        height: Math.min(col.height + 16, panel.height - 24)
        radius: 16
        color: Theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(Theme.outlineVariant, 0.6)
        scale: openState ? 1 : 0.92
        Behavior on scale {
            NumberAnimation {
                duration: 220
                easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
            }
        }

        Shadow {
            extent: 8
            radius: 24
        }

        MouseArea { anchors.fill: parent }

        Flickable {
            anchors.fill: parent
            anchors.margins: 8
            contentHeight: col.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: col
                width: parent.width
                spacing: 2

                Item {
                    width: parent.width
                    height: menu.canBack ? 40 : 0
                    visible: menu.canBack

                    Rectangle {
                        anchors.fill: parent
                        radius: 20
                        color: backHover.containsMouse ? Qt.alpha(Theme.textOnSurface, 0.08) : "transparent"
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        x: 8
                        spacing: 10

                        Item {
                            width: 32
                            height: 32
                            anchors.verticalCenter: parent.verticalCenter

                            MIcon {
                                anchors.centerIn: parent
                                width: 18
                                height: 18
                                path: Theme.iconBack
                                color: Theme.onSurfaceVariant
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Back")
                            font.pixelSize: 13
                            color: Theme.onSurfaceVariant
                        }
                    }

                    MouseArea {
                        id: backHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: menu.back()
                    }
                }

                Repeater {
                    model: menu.items

                    delegate: Item {
                        id: entry
                        required property var modelData
                        width: col.width
                        height: entry.modelData.sep ? 9 : 44

                        Rectangle {
                            visible: entry.modelData.sep
                            anchors.centerIn: parent
                            width: parent.width - 16
                            height: 1
                            color: Theme.outlineVariant
                        }

                        Item {
                            visible: !entry.modelData.sep
                            anchors.fill: parent
                            opacity: entry.modelData.enabled ? 1 : 0.4

                            Rectangle {
                                anchors.fill: parent
                                radius: 22
                                color: entryHover.containsMouse && entry.modelData.enabled
                                       ? Qt.alpha(Theme.textOnSurface, 0.08) : "transparent"
                                Behavior on color { ColorAnimation { duration: 120 } }
                            }

                            Item {
                                x: 6
                                width: 40
                                height: parent.height

                                Image {
                                    anchors.centerIn: parent
                                    width: 20
                                    height: 20
                                    visible: entry.modelData.hasIcon || entry.modelData.iconName !== ""
                                    source: entry.modelData.hasIcon
                                            ? menu.iconUrl(entry.modelData.id)
                                            : "image://loali/" + entry.modelData.iconName
                                    sourceSize: Qt.size(40, 40)
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }
                            }

                            Text {
                                x: 56
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - (entry.modelData.hasChildren ? 126 : 96)
                                text: entry.modelData.label
                                elide: Text.ElideRight
                                color: Theme.textOnSurface
                                font.pixelSize: 14
                            }

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                width: 20
                                height: 20
                                radius: 6
                                visible: entry.modelData.toggleType === "checkmark"
                                color: entry.modelData.toggleState === 1 ? Theme.primary : "transparent"
                                border.width: 2
                                border.color: entry.modelData.toggleState === 1 ? Theme.primary : Theme.outline

                                MIcon {
                                    anchors.centerIn: parent
                                    width: 14
                                    height: 14
                                    visible: entry.modelData.toggleState === 1
                                    path: Theme.iconCheck
                                    color: Theme.onPrimary
                                }
                            }

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                width: 20
                                height: 20
                                radius: 10
                                visible: entry.modelData.toggleType === "radio"
                                color: "transparent"
                                border.width: 2
                                border.color: entry.modelData.toggleState === 1 ? Theme.primary : Theme.outline

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 10
                                    height: 10
                                    radius: 5
                                    visible: entry.modelData.toggleState === 1
                                    color: Theme.primary
                                }
                            }

                            MIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                width: 20
                                height: 20
                                visible: entry.modelData.hasChildren
                                path: Theme.iconChevron
                                color: Theme.onSurfaceVariant
                            }

                            MouseArea {
                                id: entryHover
                                anchors.fill: parent
                                hoverEnabled: true
                                enabled: entry.modelData.enabled
                                onClicked: {
                                    if (entry.modelData.hasChildren) {
                                        menu.enter(entry.modelData.id)
                                    } else {
                                        menu.trigger(entry.modelData.id)
                                        panel.close()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
