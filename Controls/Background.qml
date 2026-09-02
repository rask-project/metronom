import QtQuick
import QtQuick.Controls.Material

Rectangle {
    id: control

    readonly property color gradientStop1: Qt.darker(Material.accent, 15)
    readonly property color gradientStop2: Qt.darker(Material.accent, 30)

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
