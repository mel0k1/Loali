pragma Singleton
import QtQuick

QtObject {
    readonly property color primary: "#D0BCFF"
    readonly property color onPrimary: "#381E72"
    readonly property color primaryContainer: "#4F378B"
    readonly property color onPrimaryContainer: "#EADDFF"
    readonly property color secondary: "#CCC2DC"
    readonly property color onSecondary: "#332D41"
    readonly property color secondaryContainer: "#4A4458"
    readonly property color tertiary: "#EFB8C8"
    readonly property color tertiaryContainer: "#633B48"
    readonly property color surface: "#141218"
    readonly property color surfaceContainerLow: "#1D1B20"
    readonly property color surfaceContainer: "#211F26"
    readonly property color surfaceContainerHigh: "#2B2930"
    readonly property color surfaceContainerHighest: "#36343B"
    readonly property color onSurface: "#E6E0E9"
    readonly property color onSurfaceVariant: "#CAC4D0"
    readonly property color outline: "#938F99"
    readonly property color outlineVariant: "#49454F"
    readonly property color error: "#F2B8B5"
    readonly property color scrim: "#000000"

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
}
