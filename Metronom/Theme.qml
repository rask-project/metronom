pragma Singleton

import QtQuick

// App colours, independent of the Qt Quick Controls style (Material elsewhere, macOS on macOS).
QtObject {
    readonly property bool dark: Appearance.dark

    // Material Orange: shade 500 on light, shade 200 on dark (as Material.accent).
    readonly property color accent: dark ? "#ffcc80" : "#ff9800"
    // Material Deep Orange 500.
    readonly property color primary: "#ff5722"

    readonly property color backgroundStart: dark ? Qt.darker(accent, 15) : Qt.lighter(accent, 2.00)
    readonly property color backgroundEnd: dark ? Qt.darker(accent, 30) : Qt.lighter(accent, 1.95)
    readonly property color headerBackground: dark ? Qt.darker(accent, 16) : Qt.lighter(accent, 1.98)
    readonly property color dialogBorder: dark ? Qt.darker(accent, 7) : Qt.lighter(accent, 1.9)
    readonly property color scrim: Qt.rgba(0, 0, 0, 0.5)

    readonly property real dialogRadius: 16
}
