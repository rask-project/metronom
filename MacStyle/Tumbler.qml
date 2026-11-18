import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

// Picker wheel with the selection band of macOS/iOS pickers behind the current item.
T.Tumbler {
    id: control

    readonly property real __delegateHeight: availableHeight / visibleItemCount

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    delegate: Text {
        required property var modelData
        required property int index

        text: modelData
        color: Tumbler.tumbler.palette.windowText
        font: Tumbler.tumbler.font
        opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    contentItem: TumblerView {
        implicitWidth: 60
        implicitHeight: 200

        model: control.model
        delegate: control.delegate
        path: Path {
            startX: control.contentItem.width / 2
            startY: -control.__delegateHeight / 2

            PathLine {
                x: control.contentItem.width / 2
                y: (control.visibleItemCount + 1) * control.__delegateHeight - control.__delegateHeight / 2
            }
        }
    }

    background: Item {
        Rectangle {
            x: control.leftPadding
            y: control.topPadding + (control.availableHeight - height) / 2
            width: control.availableWidth
            height: control.__delegateHeight

            radius: 8
            color: Qt.alpha(control.palette.windowText, 0.07)
            border.width: control.visualFocus ? 2 : 0
            border.color: control.palette.accent
        }
    }
}
