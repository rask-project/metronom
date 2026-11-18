import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

// Form row: a rounded highlight while hovered or pressed instead of the list-row background of the macOS style.
T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    leftInset: 8
    rightInset: 8
    spacing: 8

    icon.width: 16
    icon.height: 16

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.display === IconLabel.IconOnly || control.display === IconLabel.TextUnderIcon ? Qt.AlignCenter : Qt.AlignLeft

        icon: control.icon
        text: control.text
        font: control.font
        color: control.highlighted ? control.palette.accent : control.palette.windowText
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 28

        radius: 6
        color: Qt.alpha(control.palette.windowText, control.down ? 0.16 : control.hovered ? 0.08 : 0.0)
        border.width: control.visualFocus ? 2 : 0
        border.color: control.palette.accent
    }
}
