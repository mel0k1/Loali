import QtQuick

Item {
    id: shadow

    property real extent: 10
    property real radius: 32

    z: -1

    Rectangle {
        x: -shadow.extent * 0.4
        y: -shadow.extent * 0.4
        width: parent.width + shadow.extent * 0.8
        height: parent.height + shadow.extent * 0.8
        radius: shadow.radius + shadow.extent * 0.4
        color: "#2E000000"
    }
    Rectangle {
        x: -shadow.extent
        y: -shadow.extent
        width: parent.width + shadow.extent * 2
        height: parent.height + shadow.extent * 2
        radius: shadow.radius + shadow.extent
        color: "#19000000"
    }
}
