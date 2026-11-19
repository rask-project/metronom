pragma Singleton

import QtQuick

// App colours, independent of the Qt Quick Controls style (Material elsewhere, macOS on macOS).
QtObject {
    readonly property bool dark: Appearance.dark

    // Material Orange: shade 500 on light, shade 200 on dark (as Material.accent).
    readonly property color accent: dark ? "#ffcc80" : "#ff9800"
    // Fills of native controls (switch, slider): saturated on both themes, as the macOS accent colours.
    readonly property color controlAccent: "#ff9800"
    // Material primary text; the palette does not follow a theme change under Material.
    readonly property color foreground: dark ? "#ffffff" : "#dd000000"
    // Material Deep Orange 500.
    readonly property color primary: "#ff5722"
    // Timing feedback: Material Green 400 / 200 on time, Red 600 / 200 late, primary early.
    readonly property color onTime: dark ? "#a5d6a7" : "#66bb6a"
    readonly property color late: dark ? "#ef9a9a" : "#e53935"

    readonly property color backgroundStart: dark ? Qt.darker(accent, 15) : Qt.lighter(accent, 2.00)
    readonly property color backgroundEnd: dark ? Qt.darker(accent, 30) : Qt.lighter(accent, 1.95)
    readonly property color headerBackground: dark ? Qt.darker(accent, 16) : Qt.lighter(accent, 1.98)
    readonly property color dialogBorder: dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.12)
    readonly property color dialogShadow: Qt.rgba(0, 0, 0, dark ? 0.6 : 0.3)
    // Dims the blurred page behind a modal dialog.
    readonly property color dim: Qt.rgba(0, 0, 0, dark ? 0.3 : 0.12)
    readonly property color scrim: Qt.rgba(0, 0, 0, 0.5)

    readonly property real dialogRadius: 16

    function timingColor(accuracy: int): color {
        switch (accuracy) {
        case TimingAnalyzer.Early:
            return primary;
        case TimingAnalyzer.Late:
            return late;
        default:
            return onTime;
        }
    }
}
