import QtQuick

Item {
    id: root

    property var sourceItem
    property real cornerRadius: 0
    property real blurPx: 2.5
    readonly property bool glOk: GraphicsInfo.api === GraphicsInfo.Underlying
                                 || GraphicsInfo.api === GraphicsInfo.OpenGL

    visible: glOk && sourceItem !== null

    ShaderEffectSource {
        id: src
        sourceItem: root.sourceItem
        sourceRect: Qt.rect(root.x, root.y, root.width, root.height)
        textureSize: Qt.size(Math.max(1, Math.round(root.width / 2)),
                             Math.max(1, Math.round(root.height / 2)))
        live: true
        recursive: false
        smooth: true
        format: ShaderEffectSource.RGBA
        visible: false
    }

    ShaderEffect {
        anchors.fill: parent
        property var src: src
        property real texelW: root.blurPx / (src.textureSize.width || 1)
        property real texelH: root.blurPx / (src.textureSize.height || 1)
        property real effW: width
        property real effH: height
        property real rad: root.cornerRadius
        vertexShader: "blur.vert.qsb"
        fragmentShader: "blur.frag.qsb"
    }
}
