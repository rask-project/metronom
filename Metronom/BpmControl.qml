import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: root

    property Metronome metronome

    spacing: 12

    RoundButton {
        Layout.preferredWidth: 44
        Layout.preferredHeight: 44

        icon.name: "arrow-down"
        onClicked: metronome.decreaseBpm()
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter

        spacing: 0

        Label {
            text: metronome.bpm
            font.pointSize: 48
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            text: qsTr("BPM")
            font.pointSize: 16
            opacity: 0.6
        }
    }

    RoundButton {
        Layout.preferredWidth: 44
        Layout.preferredHeight: 44

        icon.name: "arrow-up"
        onClicked: metronome.increaseBpm()
    }
}
