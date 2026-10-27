import QtQuick
import QtWayland.Compositor
import Loali

Item {
    id: chrome

    required property var shellSurface
    readonly property rect place: shellSurface.placement

    x: place.x
    y: place.y
    width: place.width
    height: place.height

    ShellSurfaceItem {
        id: content
        anchors.fill: parent
        shellSurface: chrome.shellSurface
        onSurfaceDestroyed: chrome.visible = false
    }
}
