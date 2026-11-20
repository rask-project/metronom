import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root

    required property Metronome metronome

    signal timeSignatureClicked

    spacing: 12

    RowLayout {
        Layout.alignment: Qt.AlignHCenter

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
    }

    // Gaps are fractions of each cell, so long measures shrink the marks instead of losing them.
    RowLayout {
        Layout.fillWidth: true

        spacing: 0

        Repeater {
            model: root.metronome.beatsPerMeasure

            Item {
                id: beat

                required property int index
                readonly property bool active: root.metronome.running && index === root.metronome.currentBeat

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.preferredHeight: 12

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: beat.width * 0.08
                    anchors.rightMargin: beat.width * 0.08

                    spacing: 0

                    Repeater {
                        model: root.metronome.subdivisions

                        Item {
                            id: subdivision

                            required property int index

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1

                            Rectangle {
                                anchors.centerIn: parent
                                width: root.metronome.subdivisions > 1 ? parent.width * 0.7 : parent.width
                                height: subdivision.index === 0 ? 12 : 6

                                radius: Math.min(width, height) / 2
                                color: beat.index === 0 ? Theme.accent : Theme.primary
                                opacity: beat.active && subdivision.index <= root.metronome.currentSubdivision ? 1.0 : 0.3
                            }
                        }
                    }
                }
            }
        }
    }
}
