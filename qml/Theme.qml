pragma Singleton
import QtQuick
import Loali

QtObject {
    readonly property var scheme: {
        if (!Settings.autoAccent)
            return null
        const d = MatColor.dark
        return d && d.primary !== undefined ? d : null
    }

    readonly property color primary: scheme ? scheme.primary : Settings.accent
    readonly property color onPrimary: {
        const l = 0.2126 * primary.r + 0.7152 * primary.g + 0.0722 * primary.b
        return l > 0.6 ? "#1D1B20" : "#FFFFFF"
    }
    readonly property color primaryContainer: scheme ? scheme.primaryContainer : "#4F378B"
    readonly property color onPrimaryContainer: scheme ? scheme.onPrimaryContainer : "#EADDFF"
    readonly property color secondary: scheme ? scheme.secondary : "#CCC2DC"
    readonly property color onSecondary: scheme ? scheme.onSecondary : "#332D41"
    readonly property color secondaryContainer: scheme ? scheme.secondaryContainer : "#4A4458"
    readonly property color onSecondaryContainer: scheme ? scheme.onSecondaryContainer : "#EADDFF"
    readonly property color tertiary: scheme ? scheme.tertiary : "#EFB8C8"
    readonly property color tertiaryContainer: scheme ? scheme.tertiaryContainer : "#633B48"
    readonly property color surface: scheme ? scheme.surface : "#141218"
    readonly property color surfaceContainerLow: scheme ? scheme.surfaceContainerLow : "#1D1B20"
    readonly property color surfaceContainer: scheme ? scheme.surfaceContainer : "#211F26"
    readonly property color surfaceContainerHigh: scheme ? scheme.surfaceContainerHigh : "#2B2930"
    readonly property color surfaceContainerHighest: scheme ? scheme.surfaceContainerHighest : "#36343B"
    readonly property color onSurface: scheme ? scheme.onSurface : "#E6E0E9"
    readonly property color onSurfaceVariant: scheme ? scheme.onSurfaceVariant : "#CAC4D0"
    readonly property color outline: scheme ? scheme.outline : "#938F99"
    readonly property color outlineVariant: scheme ? scheme.outlineVariant : "#49454F"
    readonly property color error: scheme ? scheme.error : "#F2B8B5"
    readonly property color scrim: scheme ? scheme.scrim : "#000000"

    readonly property int cornerM: 16
    readonly property int cornerL: 28
    readonly property int cornerXL: 32

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
}
