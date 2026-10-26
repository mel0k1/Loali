import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Loali

Item {
    id: settings

    property bool open: false

    anchors.fill: parent
    opacity: open ? 1 : 0
    visible: opacity > 0
    enabled: open
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: 0.5
        MouseArea {
            anchors.fill: parent
            onClicked: settings.open = false
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(960, settings.width * 0.92)
        height: Math.min(640, settings.height * 0.88)
        radius: 32
        color: Theme.surfaceContainer
        border.width: 1
        border.color: Qt.alpha(Theme.outlineVariant, 0.6)
        scale: settings.open ? 1 : 0.92
        Behavior on scale {
            NumberAnimation {
                duration: 260
                easing { type: Easing.BezierSpline; bezierCurve: Theme.emphasized }
            }
        }

        Shadow {
            extent: 16
            radius: 32
        }

        MouseArea {
            anchors.fill: parent
        }

        readonly property var pages: [
            { id: "wallpaper", title: qsTr("Wallpaper"), glyph: Theme.iconWallpaper },
            { id: "appearance", title: qsTr("Appearance"), glyph: Theme.iconPalette },
            { id: "panels", title: qsTr("Panels"), glyph: Theme.iconTune },
            { id: "workspaces", title: qsTr("Workspaces"), glyph: Theme.iconApps },
            { id: "notifications", title: qsTr("Notifications"), glyph: Theme.iconBell },
            { id: "about", title: qsTr("About"), glyph: Theme.iconInfo }
        ]

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            Item {
                width: parent.width
                height: 40

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    MIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 22
                        height: 22
                        path: Theme.iconSettings
                        color: Theme.onSurface
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Settings")
                        font.pixelSize: 20
                        font.weight: Font.Medium
                        color: Theme.onSurface
                    }
                }

                Rectangle {
                    id: closeBtn
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: 36
                    radius: 18
                    color: closeMa.containsMouse ? Theme.surfaceContainerHighest : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }

                    MIcon {
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        path: Theme.iconClose
                        color: Theme.onSurface
                    }

                    MouseArea {
                        id: closeMa
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: settings.open = false
                    }
                }
            }

            Row {
                width: parent.width
                height: parent.height - 54
                spacing: 18

                ColumnLayout {
                    id: rail
                    width: 216
                    spacing: 6

                    Repeater {
                        model: panel.pages

                        delegate: Rectangle {
                            id: navItem
                            required property var modelData
                            required property int index
                            Layout.preferredWidth: rail.width
                            Layout.preferredHeight: 46
                            radius: 23
                            readonly property bool selected: pageStack.currentIndex === navItem.index
                            color: navItem.selected ? Theme.secondaryContainer
                                                    : navMa.containsMouse ? Qt.alpha(Theme.onSurface, 0.08)
                                                    : "transparent"
                            Behavior on color { ColorAnimation { duration: 150 } }

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                x: 16
                                spacing: 12

                                MIcon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 20
                                    height: 20
                                    path: navItem.modelData.glyph
                                    color: navItem.selected ? Theme.onSecondaryContainer : Theme.onSurfaceVariant
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: navItem.modelData.title
                                    font.pixelSize: 14
                                    font.weight: navItem.selected ? Font.Medium : Font.Normal
                                    color: navItem.selected ? Theme.onSecondaryContainer : Theme.onSurface
                                }
                            }

                            MouseArea {
                                id: navMa
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: pageStack.currentIndex = navItem.index
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                StackLayout {
                    id: pageStack
                    width: parent.width - rail.width - 18
                    height: parent.height

                    WallpaperPage { }
                    AppearancePage { }
                    PanelsPage { }
                    WorkspacesPage { }
                    NotificationsPage { }
                    AboutPage { }
                }
            }
        }

        component PageTitle: Text {
            text: ""
            font.pixelSize: 22
            font.weight: Font.Medium
            color: Theme.onSurface
        }

        component SectionLabel: Text {
            text: ""
            font.pixelSize: 14
            font.weight: Font.Medium
            color: Theme.onSurfaceVariant
        }

        component MButton: Rectangle {
            id: mb
            property string label
            signal clicked()
            width: mbLbl.width + 32
            height: 40
            radius: 20
            color: mbMa.containsMouse ? Qt.alpha(Theme.primary, 0.85) : Theme.primary
            Behavior on color { ColorAnimation { duration: 120 } }
            Text {
                id: mbLbl
                anchors.centerIn: parent
                text: mb.label
                color: Theme.onPrimary
                font.pixelSize: 14
                font.weight: Font.Medium
            }
            MouseArea {
                id: mbMa
                anchors.fill: parent
                hoverEnabled: true
                onClicked: mb.clicked()
            }
        }

        component MSwitch: Rectangle {
            id: sw
            property bool checked: false
            signal toggled()
            width: 52
            height: 32
            radius: 16
            color: checked ? Theme.primary : Theme.surfaceContainerHighest
            border.width: checked ? 0 : 2
            border.color: Theme.outline
            Behavior on color { ColorAnimation { duration: 150 } }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                x: sw.checked ? sw.width - width - 4 : 4
                width: sw.checked ? 24 : 16
                height: width
                radius: width / 2
                color: sw.checked ? Theme.onPrimary : Theme.outline
                Behavior on x { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: 150 } }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: sw.toggled()
            }
        }

        component MSegments: Rectangle {
            id: seg
            property var labels: []
            property int current: 0
            signal picked(int idx)
            width: segRow.width + 8
            height: 40
            radius: 20
            color: Theme.surfaceContainerHighest
            Behavior on color { ColorAnimation { duration: 150 } }

            Row {
                id: segRow
                anchors.centerIn: parent
                spacing: 4

                Repeater {
                    model: seg.labels

                    delegate: Rectangle {
                        id: segOpt
                        required property var modelData
                        required property int index
                        width: segOptLbl.width + 24
                        height: 32
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 16
                        color: seg.current === segOpt.index ? Theme.primary : "transparent"
                        Behavior on color { ColorAnimation { duration: 180 } }

                        Text {
                            id: segOptLbl
                            anchors.centerIn: parent
                            text: segOpt.modelData
                            font.pixelSize: 13
                            font.weight: seg.current === segOpt.index ? Font.Medium : Font.Normal
                            color: seg.current === segOpt.index ? Theme.onPrimary : Theme.onSurfaceVariant
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: seg.picked(segOpt.index)
                        }
                    }
                }
            }
        }

        component SettingRow: Row {
            id: srow
            property string title
            property string subtitle
            property alias control: slot.data
            spacing: 14

            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: 320
                spacing: 2

                Text {
                    text: srow.title
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: Theme.onSurface
                }

                Text {
                    visible: srow.subtitle !== ""
                    text: srow.subtitle
                    font.pixelSize: 12
                    color: Theme.onSurfaceVariant
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }

            Item {
                id: slot
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        component WallpaperPage: ColumnLayout {
            spacing: 16

            PageTitle { text: qsTr("Wallpaper") }

            Row {
                spacing: 14

                Repeater {
                    model: [
                        { id: "dusk", name: qsTr("Dusk") },
                        { id: "bloom", name: qsTr("Bloom") },
                        { id: "dunes", name: qsTr("Dunes") },
                        { id: "meadow", name: qsTr("Meadow") }
                    ]

                    delegate: Item {
                        id: thumb
                        required property var modelData
                        width: 156
                        height: 118

                        Rectangle {
                            id: frame
                            anchors.top: parent.top
                            width: 156
                            height: 92
                            radius: 12
                            clip: true
                            color: Theme.surfaceContainerHighest
                            border.width: thumb.modelData.id === Settings.wallpaper ? 2 : 1
                            border.color: thumb.modelData.id === Settings.wallpaper ? Theme.primary
                                                                                    : Qt.alpha(Theme.outlineVariant, 0.6)
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            Wallpaper {
                                anchors.fill: parent
                                style: thumb.modelData.id
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 28
                                height: 28
                                radius: 14
                                visible: thumb.modelData.id === Settings.wallpaper
                                color: Theme.primary

                                MIcon {
                                    anchors.centerIn: parent
                                    width: 16
                                    height: 16
                                    path: Theme.iconCheck
                                    color: Theme.onPrimary
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: Settings.wallpaper = thumb.modelData.id
                            }
                        }

                        Text {
                            anchors.top: frame.bottom
                            anchors.topMargin: 6
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: thumb.modelData.name
                            font.pixelSize: 12
                            color: Theme.onSurfaceVariant
                        }
                    }
                }
            }

            SectionLabel { text: qsTr("Custom image") }

            Row {
                spacing: 12

                TextField {
                    id: customPath
                    width: 420
                    height: 40
                    placeholderText: "/path/to/wallpaper.jpg"
                    color: Theme.onSurface
                    placeholderTextColor: Theme.onSurfaceVariant
                    font.pixelSize: 14
                    text: Settings.wallpaper.indexOf("file:") === 0 ? Settings.wallpaper.substring(5) : ""
                    background: Rectangle {
                        radius: 20
                        color: Theme.surfaceContainerHighest
                        border.width: 1
                        border.color: customPath.activeFocus ? Theme.primary : Qt.alpha(Theme.outlineVariant, 0.6)
                    }
                }

                MButton {
                    label: qsTr("Set")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        if (customPath.text.trim() !== "")
                            Settings.wallpaper = "file:" + customPath.text.trim()
                    }
                }
            }

            SectionLabel {
                text: qsTr("Dim overlay: %1%").arg(Math.round(Settings.dim * 100))
            }

            Slider {
                id: dimSlider
                width: 420
                from: 0.0
                to: 0.6
                stepSize: 0.05
                value: Settings.dim
                onMoved: Settings.dim = value
            }
        }

        component AppearancePage: ColumnLayout {
            spacing: 16

            PageTitle { text: qsTr("Appearance") }

            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Dark theme")
                subtitle: qsTr("Switch between dark and light Material color schemes")
                control: MSwitch {
                    checked: Settings.dark
                    onToggled: Settings.dark = !Settings.dark
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Dynamic color")
                subtitle: qsTr("Material You palette extracted from the wallpaper")
                control: MSwitch {
                    checked: Settings.autoAccent
                    onToggled: Settings.autoAccent = !Settings.autoAccent
                }
            }

            SectionLabel {
                visible: Settings.autoAccent && MatColor.suggested.length > 0
                text: qsTr("Suggested from wallpaper")
            }

            Row {
                visible: Settings.autoAccent && MatColor.suggested.length > 0
                spacing: 12

                Repeater {
                    model: MatColor.suggested

                    delegate: Item {
                        id: sug
                        required property var modelData
                        width: 46
                        height: 54

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 40
                            height: 40
                            radius: 20
                            color: sug.modelData
                            border.width: 2
                            border.color: Qt.alpha(Theme.outlineVariant, 0.6)

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    Settings.accent = sug.modelData
                                    Settings.autoAccent = false
                                }
                            }
                        }
                    }
                }
            }

            SectionLabel { text: qsTr("Accent color") }

            Flow {
                spacing: 14
                width: parent.width

                Repeater {
                    model: [
                        { c: "#D0BCFF", n: qsTr("Violet") },
                        { c: "#A8C7FA", n: qsTr("Blue") },
                        { c: "#7FD6C2", n: qsTr("Teal") },
                        { c: "#9CD67D", n: qsTr("Green") },
                        { c: "#E6C65A", n: qsTr("Lemon") },
                        { c: "#FFB77C", n: qsTr("Orange") },
                        { c: "#EFB8C8", n: qsTr("Pink") },
                        { c: "#D8B4E2", n: qsTr("Mauve") }
                    ]

                    delegate: Item {
                        id: swatch
                        required property var modelData
                        width: 52
                        height: 64

                        Rectangle {
                            id: dot
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            width: 46
                            height: 46
                            radius: 23
                            color: swatch.modelData.c
                            border.width: 2
                            border.color: Qt.alpha(Theme.outlineVariant, 0.6)

                            Rectangle {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                radius: 11
                                visible: Qt.color(swatch.modelData.c) === Settings.accent
                                color: Qt.alpha(Theme.surface, 0.55)

                                MIcon {
                                    anchors.centerIn: parent
                                    width: 14
                                    height: 14
                                    path: Theme.iconCheck
                                    color: Theme.onSurface
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: Settings.accent = swatch.modelData.c
                            }
                        }

                        Text {
                            anchors.top: dot.bottom
                            anchors.topMargin: 4
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: swatch.modelData.n
                            font.pixelSize: 11
                            color: Theme.onSurfaceVariant
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: qsTr("The accent color tints the dock, taskbar, highlights and window focus ring.")
                wrapMode: Text.WordWrap
                font.pixelSize: 13
                color: Theme.onSurfaceVariant
            }
        }

        component PanelsPage: ColumnLayout {
            spacing: 16

            PageTitle { text: qsTr("Panels") }

            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Taskbar style")
                subtitle: qsTr("Floating pill that widens only when tasks overflow, or a classic full-width bar")
                control: MSegments {
                    labels: [qsTr("Floating"), qsTr("Full")]
                    current: Settings.taskbarStyle
                    onPicked: (i) => Settings.taskbarStyle = i
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Dock look")
                subtitle: qsTr("Dynamic Island that morphs on hover and app launches, or a flat plane across the top")
                control: MSegments {
                    labels: [qsTr("Island"), qsTr("Plane")]
                    current: Settings.dockMode
                    onPicked: (i) => Settings.dockMode = i
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Panel size")
                subtitle: qsTr("Scales the dock, taskbar and status pill together")
                control: Slider {
                    id: scaleSlider
                    width: 260
                    from: 0.7
                    to: 1.3
                    stepSize: 0.05
                    value: Settings.panelScale
                    onMoved: Settings.panelScale = value
                }
            }

            SectionLabel {
                text: qsTr("Size: %1%").arg(Math.round(Settings.panelScale * 100))
            }

            Text {
                width: parent.width
                text: qsTr("Windows keep their maximized area in sync automatically when panels change size or style.")
                wrapMode: Text.WordWrap
                font.pixelSize: 13
                color: Theme.onSurfaceVariant
            }
        }

        component NotificationsPage: ColumnLayout {
            spacing: 16

            PageTitle { text: qsTr("Notifications") }

            SettingRow {
                Layout.fillWidth: true
                title: qsTr("Do Not Disturb")
                subtitle: qsTr("Hide notification popups; alerts are delivered silently")
                control: MSwitch {
                    checked: Settings.dnd
                    onToggled: Settings.dnd = !Settings.dnd
                }
            }

            Text {
                width: parent.width
                text: qsTr("Loali implements the freedesktop Notifications daemon. Apps can send alerts, actions and icons; critical notifications stay on screen until dismissed.")
                wrapMode: Text.WordWrap
                font.pixelSize: 13
                color: Theme.onSurfaceVariant
            }
        }

        component WorkspacesPage: ColumnLayout {
            spacing: 16

            PageTitle { text: qsTr("Workspaces") }

            SectionLabel { text: qsTr("Number of workspaces") }

            Row {
                spacing: 10

                Repeater {
                    model: [2, 3, 4, 5, 6]

                    delegate: Rectangle {
                        id: chip
                        required property int modelData
                        width: 48
                        height: 40
                        radius: 20
                        readonly property bool selected: Settings.workspaces === chip.modelData
                        color: chip.selected ? Theme.secondaryContainer
                                             : chipMa.containsMouse ? Theme.surfaceContainerHighest
                                             : Theme.surfaceContainerHigh
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Text {
                            anchors.centerIn: parent
                            text: chip.modelData
                            font.pixelSize: 15
                            font.weight: chip.selected ? Font.Medium : Font.Normal
                            color: chip.selected ? Theme.onSecondaryContainer : Theme.onSurface
                        }

                        MouseArea {
                            id: chipMa
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: Settings.workspaces = chip.modelData
                        }
                    }
                }
            }

            SectionLabel { text: qsTr("Keyboard shortcuts") }

            ColumnLayout {
                spacing: 8
                width: Math.min(560, parent.width)

                Repeater {
                    model: [
                        { k: "Super + 1…6", d: qsTr("Switch workspace") },
                        { k: "Super + Shift + 1…6", d: qsTr("Move window to workspace") },
                        { k: "Super + ← / →", d: qsTr("Snap window left / right half") },
                        { k: "Super + ↑", d: qsTr("Maximize") },
                        { k: "Super + ↓", d: qsTr("Restore / minimize") },
                        { k: "Super + Alt + ←↑→↓", d: qsTr("Snap to a corner") },
                        { k: "Super + F", d: qsTr("Toggle fullscreen") },
                        { k: "Super + D", d: qsTr("Show desktop") },
                        { k: "Super + A", d: qsTr("App launcher") },
                        { k: "Super + S", d: qsTr("Settings") },
                        { k: "Alt + F4", d: qsTr("Close window") },
                        { k: "Ctrl + Alt + L", d: qsTr("Lock screen") }
                    ]

                    delegate: RowLayout {
                        id: keyRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 14

                        Rectangle {
                            Layout.preferredWidth: Math.max(150, keyLabel.width + 20)
                            Layout.preferredHeight: 30
                            radius: 8
                            color: Theme.surfaceContainerHighest

                            Text {
                                id: keyLabel
                                anchors.centerIn: parent
                                text: keyRow.modelData.k
                                font.pixelSize: 12
                                font.family: "monospace"
                                color: Theme.onSurface
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: keyRow.modelData.d
                            font.pixelSize: 13
                            color: Theme.onSurfaceVariant
                        }
                    }
                }
            }
        }

        component AboutPage: ColumnLayout {
            spacing: 10

            Item { Layout.fillHeight: true }

            Image {
                Layout.alignment: Qt.AlignHCenter
                width: 96
                height: 96
                source: "qrc:/Loali/assets/logo.svg"
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Hi! Loali"
                font.pixelSize: 28
                font.weight: Font.Medium
                color: Theme.onSurface
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Material 3 Expressive desktop environment")
                font.pixelSize: 14
                color: Theme.onSurfaceVariant
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Version %1").arg(Qt.application.version)
                font.pixelSize: 13
                color: Theme.onSurfaceVariant
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Qt 6 · Wayland · C++ / QML"
                font.pixelSize: 13
                color: Theme.onSurfaceVariant
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "GPL-3.0 · github.com/mel0k1/Loali"
                font.pixelSize: 13
                color: Theme.onSurfaceVariant
            }

            Item { Layout.fillHeight: true }
        }
    }
}
