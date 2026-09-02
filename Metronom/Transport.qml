import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

RowLayout {
    id: root

    required property Metronome metronome

    spacing: 12

    Button {
        height: 48
        icon.name: metronome.running ? "pause" : "play"
        flat: true
        highlighted: true

        onClicked: metronome.running ? metronome.stop() : metronome.start()
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        spacing: 4

        Repeater {
            model: metronome.beatsPerMeasure

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 8

                radius: 4
                color: Material.primary
                opacity: index === metronome.currentBeat ? 1.0 : 0.3
            }
        }
    }
}
