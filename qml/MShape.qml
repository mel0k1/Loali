import QtQuick
import QtQuick.Shapes
import "MaterialShapes.js" as Shapes

Item {
    id: root

    property string shape: "cookie4"
    property color color: Theme.primary
    property color trackColor: "transparent"
    property real fillProgress: 1
    property bool allowSquare: false

    readonly property string pathData: Shapes.buildPath(shape, width, height, allowSquare)

    LinearGradient {
        id: progressGradient
        x1: 0
        y1: 0
        x2: 0
        y2: root.height
        GradientStop {
            position: 0
            color: root.trackColor
        }
        GradientStop {
            position: 1 - Math.max(0, Math.min(1, root.fillProgress))
            color: root.trackColor
        }
        GradientStop {
            position: 1 - Math.max(0, Math.min(1, root.fillProgress))
            color: root.color
        }
        GradientStop {
            position: 1
            color: root.color
        }
    }

    Shape {
        id: shapeRenderer
        anchors.fill: parent
        asynchronous: false

        Component.onCompleted: {
            if (Shape.CurveRenderer !== undefined)
                shapeRenderer.preferredRendererType = Shape.CurveRenderer
        }

        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillColor: root.color
            fillGradient: root.fillProgress >= 1 ? null : progressGradient

            PathSvg {
                path: root.pathData
            }
        }
    }
}
