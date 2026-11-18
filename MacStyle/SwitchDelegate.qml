import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.macOS.impl
import QtQuick.Templates as T

// Form row: the macOS switch, without the list-row background of the macOS style.
T.SwitchDelegate {
    id: switchDelegate

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: 16
    rightPadding: 16
    topPadding: 8
    bottomPadding: 8
    spacing: 8

    icon.width: 16
    icon.height: 16

    indicator: SwitchIndicator {
        x: switchDelegate.text ? (switchDelegate.mirrored ? switchDelegate.leftPadding : switchDelegate.width - width - switchDelegate.rightPadding) : switchDelegate.leftPadding + (switchDelegate.availableWidth - width) / 2
        y: switchDelegate.topPadding + (switchDelegate.availableHeight - height) / 2
        control: switchDelegate
    }

    contentItem: IconLabel {
        leftPadding: switchDelegate.mirrored ? switchDelegate.indicator.width + switchDelegate.spacing : 0
        rightPadding: !switchDelegate.mirrored ? switchDelegate.indicator.width + switchDelegate.spacing : 0

        spacing: switchDelegate.spacing
        mirrored: switchDelegate.mirrored
        display: switchDelegate.display
        alignment: switchDelegate.display === IconLabel.IconOnly || switchDelegate.display === IconLabel.TextUnderIcon ? Qt.AlignCenter : Qt.AlignLeft

        icon: switchDelegate.icon
        text: switchDelegate.text
        font: switchDelegate.font
        color: switchDelegate.palette.windowText
    }

    background: Item {
        implicitWidth: 100
        implicitHeight: 28
    }
}
