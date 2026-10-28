import QtQuick
import Loali

Item {
    id: tint

    property var wallpaperItem: null
    property color tintColor: Theme.surfaceContainer
    property real tintAlpha: 0.7
    property int cornerRadius: 24

    readonly property point origin: {
        if (!wallpaperItem)
            return Qt.point(0, 0)
        const px = x
        const py = y
        const pw = width
        const ph = height
        return mapToItem(wallpaperItem, 0, 0)
    }
    readonly property string bgKey: {
        if (!wallpaperItem || width < 2 || height < 2)
            return ""
        const style = Settings.wallpaper
        const pal = origin.x.toFixed(0) + "|" + origin.y.toFixed(0) + "|" +
                    width.toFixed(0) + "|" + height.toFixed(0) + "|" +
                    wallpaperItem.width.toFixed(0) + "|" + wallpaperItem.height.toFixed(0)
        const hex = (v) => ("0" + Math.round(v * 255).toString(16)).slice(-2)
        const conf = cornerRadius.toFixed(0) + "|" +
                     hex(tintColor.r) + hex(tintColor.g) + hex(tintColor.b) + "|" +
                     tintAlpha.toFixed(2)
        return style + "|" + pal + "|" + conf
    }

    Image {
        anchors.fill: parent
        visible: tint.bgKey !== ""
        source: tint.bgKey !== "" ? "image://loalibg/" + encodeURIComponent(tint.bgKey) : ""
        cache: false
        asynchronous: false
        fillMode: Image.Stretch
    }
}
