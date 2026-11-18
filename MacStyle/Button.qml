import QtQuick
import QtQuick.Controls.impl
import QtQuick.NativeStyle as NativeStyle
import QtQuick.Templates as T

// Push buttons are drawn by AppKit, as in the macOS style. Flat ones are borderless (AppKit would draw a pressed
// bezel for highlighted): a rounded highlight while hovered or pressed and the accent colour when highlighted.
T.Button {
    id: button

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    leftPadding: button.flat ? 8 : nativeBackground.contentPadding.left
    rightPadding: button.flat ? 8 : nativeBackground.contentPadding.right
    topPadding: button.flat ? 6 : nativeBackground.contentPadding.top
    bottomPadding: button.flat ? 6 : nativeBackground.contentPadding.bottom

    spacing: 6

    icon.width: 24
    icon.height: 24

    contentItem: IconLabel {
        spacing: button.spacing
        mirrored: button.mirrored
        display: button.display

        icon: button.icon
        defaultIconColor: {
            if (!button.flat) {
                return button.palette.buttonText;
            }
            return button.highlighted || button.checked ? button.palette.accent : button.palette.windowText;
        }
        text: button.text
        font: button.font
        color: defaultIconColor
        opacity: button.enabled ? 1.0 : 0.35
    }

    background: Item {
        implicitWidth: button.flat ? 28 : nativeBackground.implicitWidth
        implicitHeight: button.flat ? 28 : nativeBackground.implicitHeight

        NativeStyle.Button {
            id: nativeBackground

            anchors.fill: parent
            visible: !button.flat

            control: button
            contentWidth: button.contentItem.implicitWidth
            contentHeight: button.contentItem.implicitHeight
        }

        Rectangle {
            anchors.fill: parent
            visible: button.flat

            radius: 6
            color: Qt.alpha(button.palette.windowText, button.down ? 0.16 : button.hovered ? 0.08 : 0.0)
            border.width: button.visualFocus ? 2 : 0
            border.color: button.palette.accent
        }
    }
}
