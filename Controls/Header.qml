import QtQuick
import QtQuick.Controls.Material
import QtQuick.Templates as T
import QtQuick.Layouts

T.ToolBar {
    id: control

    required property string title
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
        readonly property color baseColor: Material.theme === Material.Dark ? Qt.darker(Material.accent, 16) : Qt.lighter(Material.accent, 1.98)

        implicitHeight: control.translucent ? Math.max(48, control.SafeArea.margins.top) : 48
        color: control.translucent ? Qt.alpha(baseColor, 0.25) : baseColor
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
            // font.bold: true
            font.weight: Font.Medium

            verticalAlignment: Qt.AlignVCenter
            elide: Label.ElideRight
            wrapMode: Label.WordWrap
        }

        Item {
            visible: control.settingsButton !== null

            Layout.preferredWidth: 48
            Layout.preferredHeight: 48

            children: [control.settingsButton]
        }
    }
}
