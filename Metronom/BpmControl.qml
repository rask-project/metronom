import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root

    required property Metronome metronome
    readonly property bool enableDecrease: metronome.bpm > metronome.tempoMarkings[0]
    readonly property bool enableIncrease: metronome.bpm < metronome.tempoMarkings[metronome.tempoMarkings.length - 1]

    RowLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter

        RoundButton {
            enabled: root.enableDecrease

            Layout.preferredWidth: 44
            Layout.preferredHeight: 44

            flat: true
            icon.name: "arrow-down-double"
            onClicked: metronome.previousTempoMarking()
        }

        RoundButton {
            enabled: root.enableDecrease
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44

            icon.name: "arrow-down"
            onClicked: metronome.decreaseBpm()
        }

        Label {
            Layout.preferredWidth: 150

            text: metronome.bpm
            font.pointSize: 48
            horizontalAlignment: Text.AlignHCenter
        }

        RoundButton {
            enabled: root.enableIncrease

            Layout.preferredWidth: 44
            Layout.preferredHeight: 44

            icon.name: "arrow-up"
            onClicked: metronome.increaseBpm()
        }

        RoundButton {
            enabled: root.enableIncrease

            Layout.preferredWidth: 44
            Layout.preferredHeight: 44

            flat: true
            icon.name: "arrow-up-double"
            onClicked: metronome.nextTempoMarking()
        }
    }

    Label {
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

        text: qsTr("BPM")
        font.pointSize: 16
        opacity: 0.6
    }

    Label {
        Layout.alignment: Qt.AlignHCenter

        text: metronome.tempoMarking
        font.pointSize: 10
        opacity: 0.6
    }
}
