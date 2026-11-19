import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root

    required property Metronome metronome

    spacing: 8

    GridLayout {
        Layout.fillWidth: true

        columns: 2
        columnSpacing: 12
        rowSpacing: 8

        Label {
            text: qsTr("Output")
            opacity: 0.6
        }

        ComboBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 44

            model: root.metronome.audioDeviceNames
            currentIndex: root.metronome.currentAudioDevice
            onActivated: index => root.metronome.currentAudioDevice = index
        }

        Label {
            visible: root.metronome.timing.enabled
            text: qsTr("Input")
            opacity: 0.6
        }

        ComboBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 44

            visible: root.metronome.timing.enabled
            model: root.metronome.timing.inputDeviceNames
            currentIndex: root.metronome.timing.currentInputDevice
            onActivated: index => root.metronome.timing.currentInputDevice = index
        }
    }

    Button {
        Layout.alignment: Qt.AlignHCenter

        text: qsTr("Reload audio devices")
        flat: true

        onClicked: root.metronome.refreshAudioDevices()
    }
}
