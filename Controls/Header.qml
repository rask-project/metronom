import QtQuick
import QtQuick.Controls.Material
import QtQuick.Templates as T
import QtQuick.Layouts

T.ToolBar {
    id: control

    required property string title
    property ToolButton settingsButton

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: Qt.platform.os === "android" ? 16 : 8
    topPadding: SafeArea.margins.top + 8
    bottomPadding: SafeArea.margins.bottom + 8

    background: Rectangle {
        implicitHeight: 48
        color: Material.theme === Material.Dark ? Qt.darker(Material.accent, 16) : Qt.lighter(Material.accent, 1.98)
    }

    RowLayout {
        anchors.fill: parent

        Image {
            id: image

            Layout.preferredWidth: 48
            Layout.preferredHeight: 48

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
