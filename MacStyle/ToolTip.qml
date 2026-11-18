import QtQuick
import QtQuick.Templates as T

T.ToolTip {
    id: control

    x: parent ? (parent.width - implicitWidth) / 2 : 0
    y: -implicitHeight - 4

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    margins: 6
    topPadding: 4
    bottomPadding: 4
    leftPadding: 8
    rightPadding: 8

    font.pixelSize: 11

    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent

    contentItem: Text {
        text: control.text
        font: control.font
        wrapMode: Text.Wrap
        color: control.palette.windowText
    }

    background: Rectangle {
        radius: 6
        color: control.palette.window
        border.width: 1
        border.color: Qt.alpha(control.palette.windowText, 0.15)
    }
}
