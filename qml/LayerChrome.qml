import QtQuick
import QtWayland.Compositor
import Loali

Item {
    id: chrome

    required property var shellSurface
    property var wallpaperItem: null
    property var blurSourceItem: null
    readonly property rect place: shellSurface.placement

    x: place.x
    y: place.y
    width: place.width
    height: place.height

    BlurBackdrop {
        anchors.fill: parent
        cornerRadius: 18
        sourceItem: chrome.blurSourceItem
        visible: LoaliBlur.blurred(chrome.shellSurface.surface) && chrome.blurSourceItem !== null
    }

    WallpaperTint {
        anchors.fill: parent
        wallpaperItem: chrome.wallpaperItem
        cornerRadius: 18
        tintColor: Theme.surfaceContainer
        tintAlpha: LoaliBlur.blurred(chrome.shellSurface.surface) ? 0.3 : 0.55
    }

    ShellSurfaceItem {
        id: content
        anchors.fill: parent
        shellSurface: chrome.shellSurface
        onSurfaceDestroyed: chrome.visible = false
    }
}
