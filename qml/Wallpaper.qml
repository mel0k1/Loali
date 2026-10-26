import QtQuick

Item {
    id: wallpaper

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#17131F" }
            GradientStop { position: 1.0; color: "#0F0E14" }
        }
    }

    Canvas {
        anchors.fill: parent
        onPaint: wallpaper.paint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    function paint() {
        const ctx = getContext("2d")
        ctx.reset()
        const r = Math.max(width, height)
        blob(ctx, width * 0.85, height * 0.18, r * 0.55, "#5A4A9F", 0.42)
        blob(ctx, width * 0.08, height * 0.82, r * 0.6, "#7A4A5E", 0.38)
        blob(ctx, width * 0.5, height * 0.6, r * 0.45, "#3E4A7A", 0.3)
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
