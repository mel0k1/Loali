import QtQuick
import QtQuick.Shapes
import Loali

Item {
    id: root

    property string path: ""
    property color color: Theme.onSurfaceVariant

    implicitWidth: 24
    implicitHeight: 24

    Shape {
        anchors.fill: parent
        transform: Scale {
            xScale: root.width / 24
            yScale: root.height / 24
        }
        ShapePath {
            fillColor: root.color
            strokeColor: "transparent"
            fillRule: ShapePath.OddEvenFill
            PathSvg {
                path: root.path
            }
        }
    }
}
