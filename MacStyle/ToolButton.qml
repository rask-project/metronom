import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

// Borderless toolbar button: a rounded highlight only while hovered or pressed.
T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    spacing: 6

    icon.width: 20
    icon.height: 20

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        defaultIconColor: control.checked || control.highlighted ? control.palette.accent : control.palette.windowText
        text: control.text
        font: control.font
        color: defaultIconColor
        opacity: control.enabled ? 1.0 : 0.35
    }

    background: Rectangle {
        implicitWidth: 28
        implicitHeight: 28

        radius: 6
        color: Qt.alpha(control.palette.windowText, control.down ? 0.16 : control.hovered ? 0.08 : 0.0)
        border.width: control.visualFocus ? 2 : 0
        border.color: control.palette.accent
    }
}
