import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: root

    required property Metronome metronome

    signal timeSignatureClicked

    spacing: 12

    Button {
        height: 48
        icon.name: root.metronome.running ? "pause" : "play"
        flat: true
        highlighted: true

        onClicked: root.metronome.toggle()
    }

    Button {
        Accessible.name: qsTr("Time signature")

        text: root.metronome.numerator + "/" + root.metronome.denominator
        flat: true
        font.pixelSize: 18
        font.bold: true

        onClicked: root.timeSignatureClicked()
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter

        spacing: 8

        Repeater {
            model: root.metronome.beatsPerMeasure

            RowLayout {
                id: beat

                required property int index
                readonly property bool active: root.metronome.running && index === root.metronome.currentBeat

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignVCenter

                spacing: 2

                Repeater {
                    model: root.metronome.subdivisions

                    Rectangle {
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: index === 0 ? 12 : 6
                        Layout.alignment: Qt.AlignVCenter

                        radius: height / 2
                        color: beat.index === 0 ? Theme.accent : Theme.primary
                        opacity: beat.active && index <= root.metronome.currentSubdivision ? 1.0 : 0.3
                    }
                }
            }
        }
    }
}
