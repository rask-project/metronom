pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

GridLayout {
    id: root

    required property Metronome metronome
    readonly property TimingAnalyzer timing: metronome.timing
    readonly property bool showHit: timing.status === TimingAnalyzer.Listening && timing.hasHit

    signal timeSignatureClicked

    columns: 3
    columnSpacing: 12
    rowSpacing: 4

    // The timing row keeps its height while the check is on, so the layout doesn't jump on play.
    Item {
        Layout.row: 0
        Layout.column: 0
        Layout.rowSpan: 2
        Layout.preferredWidth: playButton.implicitWidth
        Layout.preferredHeight: offsetLabel.implicitHeight + accuracyLabel.implicitHeight

        visible: root.timing.enabled

        Label {
            id: offsetLabel

            Accessible.name: qsTr("Last hit")

            font.pixelSize: 18
            font.bold: true
            color: Theme.timingColor(root.timing.lastAccuracy)
            text: root.showHit ? qsTr("%1 ms").arg(root.timing.lastOffset > 0 ? "+" + root.timing.lastOffset : root.timing.lastOffset) : ""
        }

        Label {
            id: accuracyLabel

            anchors.top: offsetLabel.bottom
            anchors.left: offsetLabel.left

            font.pixelSize: 11
            opacity: 0.6
            text: {
                if (!root.showHit) {
                    return "";
                }
                switch (root.timing.lastAccuracy) {
                case TimingAnalyzer.Early:
                    return qsTr("Early");
                case TimingAnalyzer.Late:
                    return qsTr("Late");
                default:
                    return qsTr("On time");
                }
            }
        }
    }

    Label {
        Layout.row: 0
        Layout.column: 2
        Layout.fillWidth: true
        // Takes the width the bars get instead of widening the layout to fit a long message on one line.
        Layout.preferredWidth: 0
        Layout.alignment: Qt.AlignBottom

        visible: root.timing.enabled
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Label.WordWrap
        maximumLineCount: 2
        elide: Label.ElideRight
        font.pixelSize: root.timing.status === TimingAnalyzer.Listening ? 16 : 11
        font.bold: root.timing.status === TimingAnalyzer.Listening
        opacity: root.timing.status === TimingAnalyzer.Listening ? 1.0 : 0.6
        text: {
            switch (root.timing.status) {
            case TimingAnalyzer.Calibrating:
                return qsTr("Calibrating… keep quiet");
            case TimingAnalyzer.CalibrationFailed:
                return qsTr("Calibration failed: the microphone has to hear the device's speaker, turn the volume up");
            case TimingAnalyzer.MicrophoneUnavailable:
                return qsTr("Microphone unavailable");
            case TimingAnalyzer.Listening:
                return root.timing.playedBpm > 0 ? qsTr("%1 BPM").arg(root.timing.playedBpm) : "";
            default:
                return "";
            }
        }
    }

    Bars {
        Layout.row: 1
        Layout.column: 2
        Layout.fillWidth: true

        // Hidden but laid out while stopped, so its space is kept for when the metronome plays.
        visible: root.timing.enabled
        opacity: root.metronome.running ? 1.0 : 0.0
        beats: root.metronome.beatsPerMeasure
        subdivisions: root.metronome.subdivisions
        hits: root.timing.status === TimingAnalyzer.Listening ? root.timing.measureHits : []
    }

    Button {
        id: playButton

        Layout.row: 2
        Layout.column: 0
        Layout.preferredHeight: 48
        icon.name: root.metronome.running ? "pause" : "play"
        flat: true
        highlighted: true

        onClicked: root.metronome.toggle()
    }

    Button {
        Layout.row: 2
        Layout.column: 1

        Accessible.name: qsTr("Time signature")

        text: root.metronome.numerator + "/" + root.metronome.denominator
        flat: true
        font.pixelSize: 18
        font.bold: true

        onClicked: root.timeSignatureClicked()
    }

    Bars {
        Layout.row: 2
        Layout.column: 2
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter

        beats: root.metronome.beatsPerMeasure
        subdivisions: root.metronome.subdivisions
        currentBeat: root.metronome.running ? root.metronome.currentBeat : -1
        currentSubdivision: root.metronome.currentSubdivision
    }

    // One bar per click: the reference shows the clicks played so far in the beat, the played bars
    // (with hits) light each click the player hit, in the colour of its accuracy.
    component Bars: RowLayout {
        id: bars

        required property int beats
        required property int subdivisions
        property int currentBeat: -1
        property int currentSubdivision: 0
        property var hits

        spacing: 8

        Repeater {
            model: bars.beats

            RowLayout {
                id: beat

                required property int index

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignVCenter

                spacing: 2

                Repeater {
                    model: bars.subdivisions

                    Rectangle {
                        required property int index
                        readonly property int accuracy: bars.hits?.[(beat.index * bars.subdivisions) + index] ?? -1
                        readonly property bool played: beat.index === bars.currentBeat && index <= bars.currentSubdivision

                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: index === 0 ? 12 : 6
                        Layout.alignment: Qt.AlignVCenter

                        radius: height / 2
                        color: {
                            if (bars.hits !== undefined) {
                                return accuracy >= 0 ? Theme.timingColor(accuracy) : Theme.foreground;
                            }
                            return beat.index === 0 ? Theme.accent : Theme.primary;
                        }
                        opacity: {
                            if (bars.hits !== undefined) {
                                return accuracy >= 0 ? 1.0 : 0.1;
                            }
                            return played ? 1.0 : 0.3;
                        }
                    }
                }
            }
        }
    }
}
