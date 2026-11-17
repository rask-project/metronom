import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Templates as T

import Metronom

Dialog {
    id: control

    padding: 24
    modal: true
    focus: true

    contentItem: ColumnLayout {
        spacing: 8

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 96
            Layout.preferredHeight: 96

            sourceSize.width: width
            sourceSize.height: height
            source: "qrc:/resources/logo.png"
        }

        Label {
            Layout.fillWidth: true

            text: "Rask Metronom"
            font.pixelSize: 22
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true

            text: qsTr("Version %1").arg(Qt.application.version)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 8

            text: qsTr("Built with Qt %1, running on Qt %2").arg(AppInfo.qtBuildVersion).arg(AppInfo.qtVersion)
            opacity: 0.7
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true

            text: "© 2026 Rask Project · " + qsTr("MIT License")
            opacity: 0.7
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 8

            text: "<a href=\"https://github.com/rask-project/metronom\">github.com/rask-project/metronom</a>"
            textFormat: Text.StyledText
            linkColor: Material.accent
            horizontalAlignment: Text.AlignHCenter

            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    background: Background {
        Material.roundedScale: Material.LargeScale

        opacity: 0.95
        border.color: Material.theme === Material.Dark ? Qt.darker(Material.accent, 10) : Qt.lighter(Material.accent, 1.8)
        border.width: 1
    }

    T.Overlay.modal: BlurOverlay {}
}
