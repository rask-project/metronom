import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root

    required property Metronome metronome

    readonly property var intervals: [30, 60, 120, 300, 600]
    readonly property var intervalLabels: ["30s", "1m", "2m", "5m", "10m"]

    spacing: 8

    SwitchDelegate {
        Layout.fillWidth: true

        text: qsTr("Auto-increment BPM")
        checked: metronome.autoIncrement
        onCheckedChanged: metronome.autoIncrement = checked
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 32
        Layout.rightMargin: 32
        Layout.topMargin: 6

        visible: metronome.autoIncrement

        Label {
            text: qsTr("Interval")
            opacity: 0.6
        }

        Slider {
            id: intervalSlider

            Layout.fillWidth: true

            from: 0
            to: root.intervals.length - 1
            stepSize: 1
            value: {
                const index = root.intervals.indexOf(metronome.autoIncrementInterval);
                return index >= 0 ? index : 1;
            }
            onMoved: metronome.autoIncrementInterval = root.intervals[Math.round(value)]
        }

        Label {
            text: root.intervalLabels[Math.round(intervalSlider.value)]
            font.bold: true
        }
    }
}
