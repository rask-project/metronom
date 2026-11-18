import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

// Circular button (NSBezelStyleCircular); flat ones only show the circle while hovered or pressed.
T.RoundButton {
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
        implicitWidth: 32
        implicitHeight: 32

        radius: control.radius
        color: {
            const pressedTint = control.down ? 0.16 : control.hovered ? 0.08 : 0.0;
            if (control.flat) {
                return Qt.alpha(control.palette.windowText, pressedTint);
            }
            return Color.blend(control.palette.button, control.palette.windowText, pressedTint);
        }
        border.width: control.visualFocus ? 2 : control.flat ? 0 : 1
        border.color: control.visualFocus ? control.palette.accent : Qt.alpha(control.palette.windowText, 0.15)
    }
}
