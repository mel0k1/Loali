pragma Singleton
import QtQuick
import Loali

QtObject {
    readonly property var scheme: {
        const s = Settings.dark ? MatColor.dark : MatColor.light
        return s && s.primary !== undefined ? s : null
    }

    readonly property var fb: Settings.dark ? darkFb : lightFb

    readonly property var darkFb: ({
        primary: "#D0BCFF", primaryContainer: "#4F378B", onPrimaryContainer: "#EADDFF",
        secondary: "#CCC2DC", onSecondary: "#332D41", secondaryContainer: "#4A4458", onSecondaryContainer: "#EADDFF",
        tertiary: "#EFB8C8", tertiaryContainer: "#633B48",
        surface: "#141218", surfaceContainerLow: "#1D1B20", surfaceContainer: "#211F26",
        surfaceContainerHigh: "#2B2930", surfaceContainerHighest: "#36343B",
        onSurface: "#E6E0E9", onSurfaceVariant: "#CAC4D0",
        outline: "#938F99", outlineVariant: "#49454F",
        error: "#F2B8B5", scrim: "#000000"
    })

    readonly property var lightFb: ({
        primary: "#6750A4", primaryContainer: "#EADDFF", onPrimaryContainer: "#21005D",
        secondary: "#625B71", onSecondary: "#FFFFFF", secondaryContainer: "#E8DEF8", onSecondaryContainer: "#1D192B",
        tertiary: "#7D5260", tertiaryContainer: "#FFD8E4",
        surface: "#FEF7FF", surfaceContainerLow: "#F7F2FA", surfaceContainer: "#F3EDF7",
        surfaceContainerHigh: "#ECE6F0", surfaceContainerHighest: "#E6E0E9",
        onSurface: "#1D1B20", onSurfaceVariant: "#49454F",
        outline: "#79747E", outlineVariant: "#CAC4D0",
        error: "#B3261E", scrim: "#000000"
    })

    readonly property color primary: scheme ? scheme.primary : fb.primary
    readonly property color onPrimary: {
        const l = 0.2126 * primary.r + 0.7152 * primary.g + 0.0722 * primary.b
        return l > 0.6 ? "#1D1B20" : "#FFFFFF"
    }
    readonly property color primaryContainer: scheme ? scheme.primaryContainer : fb.primaryContainer
    readonly property color onPrimaryContainer: scheme ? scheme.onPrimaryContainer : fb.onPrimaryContainer
    readonly property color secondary: scheme ? scheme.secondary : fb.secondary
    readonly property color onSecondary: scheme ? scheme.onSecondary : fb.onSecondary
    readonly property color secondaryContainer: scheme ? scheme.secondaryContainer : fb.secondaryContainer
    readonly property color onSecondaryContainer: scheme ? scheme.onSecondaryContainer : fb.onSecondaryContainer
    readonly property color tertiary: scheme ? scheme.tertiary : fb.tertiary
    readonly property color tertiaryContainer: scheme ? scheme.tertiaryContainer : fb.tertiaryContainer
    readonly property color surface: scheme ? scheme.surface : fb.surface
    readonly property color surfaceContainerLow: scheme ? scheme.surfaceContainerLow : fb.surfaceContainerLow
    readonly property color surfaceContainer: scheme ? scheme.surfaceContainer : fb.surfaceContainer
    readonly property color surfaceContainerHigh: scheme ? scheme.surfaceContainerHigh : fb.surfaceContainerHigh
    readonly property color surfaceContainerHighest: scheme ? scheme.surfaceContainerHighest : fb.surfaceContainerHighest
    readonly property color onSurface: scheme ? scheme.onSurface : fb.onSurface
    readonly property color onSurfaceVariant: scheme ? scheme.onSurfaceVariant : fb.onSurfaceVariant
    readonly property color outline: scheme ? scheme.outline : fb.outline
    readonly property color outlineVariant: scheme ? scheme.outlineVariant : fb.outlineVariant
    readonly property color error: scheme ? scheme.error : fb.error
    readonly property color scrim: scheme ? scheme.scrim : fb.scrim

    readonly property int cornerM: 16
    readonly property int cornerL: 28
    readonly property int cornerXL: 32

    readonly property real uiScale: Settings.panelScale

    function sz(base) {
        return Math.round(base * uiScale)
    }

    readonly property int durationS: 150
    readonly property int durationM: 300
    readonly property int durationL: 500

    readonly property list<real> emphasized: [0.2, 0, 0, 1, 1, 1]

    readonly property string iconClose: "M19 6.41 17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"
    readonly property string iconMaximize: "M18 4H6c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 14H6V6h12v12z"
    readonly property string iconMinimize: "M19 13H5v-2h14v2z"
    readonly property string iconRestore: "M17 3H7c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h10c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 12H7V5h10v10zM3 7v12c0 1.1.9 2 2 2h12v-2H5V7H3z"
    readonly property string iconApps: "M3 3v8h8V3H3zm10 0v8h8V3h-8zM3 13v8h8v-8H3zm10 0v8h8v-8h-8z"
    readonly property string iconSearch: "M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"
    readonly property string iconLock: "M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zM9 6c0-1.66 1.34-3 3-3s3 1.34 3 3v2H9V6z"
    readonly property string iconSettings: "M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"
    readonly property string iconWallpaper: "M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z"
    readonly property string iconPalette: "M12 3c-4.97 0-9 4.03-9 9s4.03 9 9 9c.83 0 1.5-.67 1.5-1.5 0-.39-.15-.74-.39-1.01-.23-.26-.38-.61-.38-.99 0-.83.67-1.5 1.5-1.5H16c2.76 0 5-2.24 5-5 0-4.42-4.03-8-9-8zm-5.5 9c-.83 0-1.5-.67-1.5-1.5S5.67 9 6.5 9 8 9.67 8 10.5 7.33 12 6.5 12zm3-4C8.67 8 8 7.33 8 6.5S8.67 5 9.5 5s1.5.67 1.5 1.5S10.33 8 9.5 8zm5 0c-.83 0-1.5-.67-1.5-1.5S13.67 5 14.5 5s1.5.67 1.5 1.5S15.33 8 14.5 8zm3 4c-.83 0-1.5-.67-1.5-1.5S16.67 9 17.5 9s1.5.67 1.5 1.5-.67 1.5-1.5 1.5z"
    readonly property string iconInfo: "M11 7h2v2h-2zm0 4h2v6h-2zm1-9C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8z"
    readonly property string iconCheck: "M9 16.17 4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"
    readonly property string iconFolder: "M10 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z"
    readonly property string iconBell: "M12 22c1.1 0 2-.9 2-2h-4c0 1.1.9 2 2 2zm6-6v-5c0-3.07-1.63-5.64-4.5-6.32V4c0-.83-.67-1.5-1.5-1.5s-1.5.67-1.5 1.5v.68C7.64 5.36 6 7.92 6 11v5l-2 2v1h16v-1l-2-2z"
    readonly property string iconBellOff: "M12 22c1.1 0 2-.9 2-2h-4c0 1.1.9 2 2 2zm6-6v-5c0-3.07-1.64-5.64-4.5-6.32V4c0-.83-.67-1.5-1.5-1.5s-1.5.67-1.5 1.5v.68c-.24.06-.47.15-.69.23l1.64 1.64c.18-.05.36-.05.55-.05 2.21 0 4 1.79 4 4v3.68l2 2v.14H8.83l2 2H18l2 2v-1l-2-2zM4.41 2.86 3 4.27l2 2V11c0 3.07-1.64 5.64-2.5 6.32V19h12.23l3.06 3.06 1.41-1.41L4.41 2.86z"
    readonly property string iconAuto: "M12 2 4.5 20.29l.71.71L12 18l6.79 3 .71-.71z"
    readonly property string iconSun: "M12 7c-2.76 0-5 2.24-5 5s2.24 5 5 5 5-2.24 5-5-2.24-5-5-5zM2 13h2c.55 0 1-.45 1-1s-.45-1-1-1H2c-.55 0-1 .45-1 1s.45 1 1 1zm18 0h2c.55 0 1-.45 1-1s-.45-1-1-1h-2c-.55 0-1 .45-1 1s.45 1 1 1zM11 2v2c0 .55.45 1 1 1s1-.45 1-1V2c0-.55-.45-1-1-1s-1 .45-1 1zm0 18v2c0 .55.45 1 1 1s1-.45 1-1v-2c0-.55-.45-1-1-1s-1 .45-1 1zM5.99 4.58a.996.996 0 0 0-1.41 0 .996.996 0 0 0 0 1.41l1.06 1.06c.39.39 1.03.39 1.41 0s.39-1.03 0-1.41L5.99 4.58zm12.37 12.37a.996.996 0 0 0-1.41 0 .996.996 0 0 0 0 1.41l1.06 1.06c.39.39 1.03.39 1.41 0a.996.996 0 0 0 0-1.41l-1.06-1.06zm1.06-10.96a.996.996 0 0 0 0-1.41.996.996 0 0 0-1.41 0l-1.06 1.06c-.39.39-.39 1.03 0 1.41s1.03.39 1.41 0l1.06-1.06zM7.05 18.36a.996.996 0 0 0 0-1.41.996.996 0 0 0-1.41 0l-1.06 1.06c-.39.39-.39 1.03 0 1.41s1.03.39 1.41 0l1.06-1.06z"
    readonly property string iconMoon: "M12 3a9 9 0 1 0 9 9c0-.46-.04-.92-.1-1.36a5.39 5.39 0 0 1-4.4 2.26 5.4 5.4 0 0 1-3.14-9.8c-.44-.06-.9-.1-1.36-.1z"
    readonly property string iconExpo: "M3 13h8V3H3v10zm0 8h8v-6H3v6zm10 0h8V11h-8v10zm0-18v6h8V3h-8z"
    readonly property string iconTune: "M3 5h6v2H3zM13 5h8v2h-8zM11 3h2v6h-2zM3 11h10v2H3zM19 11h2v2h-2zM15 9h2v6h-2zM3 17h4v2H3zM13 17h8v2h-8zM9 15h2v6H9z"
    readonly property string iconBack: "M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z"
    readonly property string iconChevron: "M10 6 8.59 7.41 13.17 12l-4.58 4.59L10 18l6-6z"
    readonly property string iconPin: "M16 9V4h1c.55 0 1-.45 1-1s-.45-1-1-1H7c-.55 0-1 .45-1 1s.45 1 1 1h1v5c0 1.66-1.34 3-3 3v2h5.97v7l1 1 1-1v-7H19v-2c-1.66 0-3-1.34-3-3z"
}
