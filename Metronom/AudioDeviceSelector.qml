import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: root

    required property Metronome metronome

    spacing: 12

    Label {
        text: qsTr("Output")
        opacity: 0.6
    }

    ComboBox {
        Layout.fillWidth: true
        Layout.preferredHeight: 44

        model: metronome.audioDeviceNames
        currentIndex: metronome.currentAudioDevice
        onActivated: index => metronome.currentAudioDevice = index
    }
}
