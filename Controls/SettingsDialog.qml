import QtCore

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Effects

import Metronom

Dialog {
    id: control

    required property Settings settings
    required property Metronome metronome

    signal aboutRequested

    padding: 0
    topPadding: 6
    bottomPadding: 32

    modal: true
    focus: true

    title: qsTr("Settings")

    header: Label {
        visible: parent?.parent === Overlay.overlay && control.title

        text: control.title

        font.bold: true
        font.pixelSize: 18
        padding: 16

        elide: Label.ElideRight
        verticalAlignment: Label.AlignVCenter
    }

    contentItem: ColumnLayout {
        spacing: 16

        SwitchDelegate {
            Layout.fillWidth: true

            text: qsTr("Dark mode")

            checked: Material.theme === Material.Dark
            onCheckedChanged: control.settings.theme = checked ? Material.Dark : Material.Light
        }

        SwitchDelegate {
            Layout.fillWidth: true

            text: qsTr("Save the BPM and note value")

            checked: control.settings.autoSave
            onCheckedChanged: control.settings.autoSave = checked
        }

        AutoIncrementControl {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            metronome: control.metronome
        }

        LanguageSelector {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            manager: LanguageManager {
                id: languageManager
            }
        }

        AudioDeviceSelector {
            id: audioDeviceSelector

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            metronome: control.metronome
        }

        ItemDelegate {
            Layout.fillWidth: true

            text: qsTr("About Rask Metronom")

            onClicked: control.aboutRequested()
        }
    }

    background: Background {
        Material.roundedScale: Material.LargeScale

        opacity: 0.95
        border.color: Material.theme === Material.Dark ? Qt.darker(Material.accent, 7) : Qt.lighter(Material.accent, 1.9)
        border.width: 1
    }

    T.Overlay.modal: BlurOverlay {}

    T.Overlay.modeless: Rectangle {
        readonly property color backgroundColor: Material.dropShadowColor

        color: Qt.rgba(backgroundColor.r, backgroundColor.g, backgroundColor.b, 0.5)
    }
}
