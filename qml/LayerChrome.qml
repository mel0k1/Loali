import QtQuick
import QtWayland.Compositor
import Loali

Item {
    id: chrome

    required property var shellSurface
    property var wallpaperItem: null
    readonly property rect place: shellSurface.placement

    x: place.x
    y: place.y
    width: place.width
    height: place.height

    WallpaperTint {
        anchors.fill: parent
        wallpaperItem: chrome.wallpaperItem
        cornerRadius: 18
        tintColor: Theme.surfaceContainer
        tintAlpha: 0.55
    }

    ShellSurfaceItem {
        id: content
        anchors.fill: parent
        shellSurface: chrome.shellSurface
        onSurfaceDestroyed: chrome.visible = false
    }
}
