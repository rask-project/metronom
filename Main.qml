import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Metronom

ApplicationWindow {
    id: window

    width: 640
    height: 480
    minimumWidth: 200
    minimumHeight: 250

    visible: true
    title: qsTr("Rask Metronom")

    Material.theme: Material.Dark
    Material.background: "#000000"

    Metronome {
        id: metronome
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.topMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16
        spacing: 16

        BpmControl {
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        NoteValueSelector {
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        Transport {
            Layout.fillWidth: true
            metronome: metronome
        }
    }
}
