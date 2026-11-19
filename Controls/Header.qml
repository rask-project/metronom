import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import Metronom

T.ToolBar {
    id: control

    required property string title
    property ToolButton helpButton
    property ToolButton settingsButton
    // macOS: the header shares the title bar and starts after the window buttons.
    property bool translucent: false
    property real leadingInset: 0

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: Qt.platform.os === "android" ? 16 : 8
    leftPadding: control.padding + control.leadingInset
    topPadding: control.translucent ? 0 : SafeArea.margins.top + 8
    bottomPadding: control.translucent ? 0 : SafeArea.margins.bottom + 8

    background: Rectangle {
        implicitHeight: control.translucent ? Math.max(48, control.SafeArea.margins.top) : 48
        color: control.translucent ? Qt.alpha(Theme.headerBackground, 0.25) : Theme.headerBackground
    }

    RowLayout {
        anchors.fill: parent

        Image {
            id: image

            Layout.preferredWidth: 42
            Layout.preferredHeight: 42

            sourceSize.width: width
            sourceSize.height: height

            source: "qrc:/resources/logo.png"
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true

            text: control.title
            font.pixelSize: 20
            font.weight: Font.Medium

            verticalAlignment: Qt.AlignVCenter
            elide: Label.ElideRight
            wrapMode: Label.WordWrap
        }

        Item {
            visible: control.helpButton !== null

            Layout.preferredWidth: control.helpButton?.implicitWidth ?? 0
            Layout.preferredHeight: 48

            children: [control.helpButton]
        }

        Item {
            visible: control.settingsButton !== null

            Layout.preferredWidth: control.settingsButton?.implicitWidth ?? 0
            Layout.preferredHeight: 48

            children: [control.settingsButton]
        }
    }
}
