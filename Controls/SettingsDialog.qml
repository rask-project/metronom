import QtCore

import QtQuick
import QtQuick.Controls
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

    // Scrolls when the dialog is capped to a short window (landscape on phones).
    contentItem: ScrollView {
        id: scrollView

        contentWidth: availableWidth

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 16

            SwitchDelegate {
                Layout.fillWidth: true

                text: qsTr("Dark mode")

                checked: Appearance.dark
                onToggled: Appearance.colorScheme = checked ? Qt.Dark : Qt.Light
            }

            SwitchDelegate {
                Layout.fillWidth: true

                text: qsTr("Save the BPM, time signature and note value")

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
    }

    background: DialogBackground {}

    T.Overlay.modal: BlurOverlay {}

    T.Overlay.modeless: Rectangle {
        color: Theme.scrim
    }
}
