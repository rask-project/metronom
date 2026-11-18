import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: root

    required property Metronome metronome
    readonly property bool enableTempoMarkingDecrease: metronome.bpm > metronome.tempoMarkings[0]
    readonly property bool enableTempoMarkingIncrease: metronome.bpm < metronome.tempoMarkings[metronome.tempoMarkings.length - 1]
    readonly property bool enableDecrease: metronome.bpm >= metronome.minBpm
    readonly property bool enableIncrease: metronome.bpm <= metronome.maxBpm

    Layout.fillWidth: true
    Layout.alignment: Qt.AlignHCenter

    RoundButton {
        enabled: root.enableTempoMarkingDecrease

        Layout.preferredWidth: 44
        Layout.preferredHeight: 44

        flat: true
        icon.name: "arrow-up-double"
        onClicked: metronome.previousTempoMarking()
    }

    RowLayout {
        Layout.preferredWidth: 200
        Layout.fillHeight: true

        Tumbler {
            id: tumbler

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight
            Layout.minimumWidth: 150
            Layout.alignment: Qt.AlignRight

            // Fewer, taller rows when the height is short (landscape on phones).
            visibleItemCount: height < 180 ? 3 : 5

            currentIndex: metronome.bpm - metronome.minBpm
            model: Array.from({
                length: (metronome.maxBpm - metronome.minBpm) + 1
            }, (_, i) => metronome.minBpm + i)

            delegate: Label {
                required property var modelData
                required property int index

                text: modelData
                opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 48 * (opacity)
            }

            onCurrentIndexChanged: metronome.bpm = currentIndex + metronome.minBpm
        }

        Column {
            Layout.fillWidth: true
            Layout.minimumWidth: 80
            Layout.alignment: Qt.AlignLeft
            spacing: 5

            Label {
                text: qsTr("BPM")
                font.pointSize: 20
                opacity: 0.6

                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: metronome.tempoMarking
                font.pointSize: 14
                opacity: 0.6

                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    RoundButton {
        enabled: root.enableTempoMarkingIncrease

        Layout.preferredWidth: 44
        Layout.preferredHeight: 44

        flat: true
        icon.name: "arrow-down-double"
        onClicked: metronome.nextTempoMarking()
    }
}
