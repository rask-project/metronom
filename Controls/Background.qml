import QtQuick
import QtQuick.Controls.Material

Rectangle {
    id: control

    readonly property bool isDark: Material.theme === Material.Dark
    readonly property color gradientStop1: control.isDark ? Qt.darker(Material.accent, 15) : Qt.lighter(Material.accent, 2.00)
    readonly property color gradientStop2: control.isDark ? Qt.darker(Material.accent, 30) : Qt.lighter(Material.accent, 1.95)

    color: control.gradientStop2
    radius: Material.roundedScale

    gradient: Gradient {
        GradientStop {
            position: 0.25
            color: control.gradientStop1
        }
        GradientStop {
            position: 1.00
            color: control.gradientStop2
        }
    }
}
