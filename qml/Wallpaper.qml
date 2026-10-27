import QtQuick
import Loali

Item {
    id: wallpaper

    property string style: Settings.wallpaper
    property real dim: Settings.dim

    readonly property bool isImage: style === "dusk" || style.indexOf("file:") === 0
    readonly property string imageSource: style === "dusk" ? "qrc:/Loali/assets/wallpapers/dusk.jpg" : style

    readonly property var palettes: ({
        bloom: { top: "#17131F", bottom: "#0F0E14", blobs: [
            { x: 0.85, y: 0.18, r: 0.55, c: "#5A4A9F", a: 0.42 },
            { x: 0.08, y: 0.82, r: 0.60, c: "#7A4A5E", a: 0.38 },
            { x: 0.50, y: 0.60, r: 0.45, c: "#3E4A7A", a: 0.30 }
        ] },
        dunes: { top: "#1F1410", bottom: "#140E0C", blobs: [
            { x: 0.20, y: 0.22, r: 0.60, c: "#B36A3C", a: 0.40 },
            { x: 0.80, y: 0.75, r: 0.55, c: "#8A4A5E", a: 0.38 },
            { x: 0.50, y: 0.45, r: 0.42, c: "#C98A4A", a: 0.28 }
        ] },
        meadow: { top: "#101710", bottom: "#0C120C", blobs: [
            { x: 0.80, y: 0.75, r: 0.60, c: "#3E7A4A", a: 0.40 },
            { x: 0.15, y: 0.20, r: 0.50, c: "#4A7A6E", a: 0.35 },
            { x: 0.50, y: 0.85, r: 0.45, c: "#6E9A3E", a: 0.30 }
        ] }
    })

    readonly property var pal: {
        if (isImage)
            return { top: "#141218", bottom: "#0F0E14", blobs: [] }
        return palettes[style] ? palettes[style] : palettes.bloom
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: wallpaper.pal.top }
            GradientStop { position: 1.0; color: wallpaper.pal.bottom }
        }
    }

    Image {
        anchors.fill: parent
        visible: wallpaper.isImage
        source: wallpaper.isImage ? wallpaper.imageSource : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        visible: !wallpaper.isImage
        onPaint: wallpaper.paint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onVisibleChanged: requestPaint()
        Connections {
            target: wallpaper
            function onStyleChanged() { requestPaint() }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: wallpaper.dim
    }

    function paint() {
        const ctx = canvas.getContext("2d")
        ctx.reset()
        const r = Math.max(width, height)
        for (const b of wallpaper.pal.blobs)
            blob(ctx, width * b.x, height * b.y, r * b.r, b.c, b.a)
    }

    function blob(ctx, x, y, radius, color, alpha) {
        const g = ctx.createRadialGradient(x, y, 0, x, y, radius)
        g.addColorStop(0, color)
        g.addColorStop(1, "rgba(0,0,0,0)")
        ctx.globalAlpha = alpha
        ctx.fillStyle = g
        ctx.beginPath()
        ctx.arc(x, y, radius, 0, Math.PI * 2)
        ctx.fill()
        ctx.globalAlpha = 1
    }
}
