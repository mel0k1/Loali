pragma Singleton
import QtQuick
import "MaterialShapes.js" as Shapes

QtObject {
    readonly property var catalog: Shapes.catalog
    readonly property var names: Shapes.names

    function path(kind, width, height, square) {
        return Shapes.buildPath(kind, width, height, square === true)
    }

    function rotationScale(kind, aspectRatio) {
        return Shapes.rotationScale(kind, aspectRatio)
    }

    function polarProfile(kind, samples) {
        return Shapes.polarProfile(kind, samples)
    }

    function morphScale(profiles) {
        return Shapes.morphScale(profiles)
    }

    function profile(kind, samples) {
        return Shapes.polarProfile(kind, samples)
    }
}
