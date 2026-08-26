import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Metronom

ApplicationWindow {
    id: window

    minimumWidth: 380
    minimumHeight: 480
    maximumWidth: 800
    maximumHeight: 600

    visible: true
    title: qsTr("Rask Metronom")
    flags: Qt.Window | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint

    readonly property bool isLandscape: width > height && width >= layout.anchors.leftMargin + layout.anchors.rightMargin + bpmControl.implicitWidth + Math.max(noteValueSelector.implicitWidth, transport.implicitWidth)

    Material.theme: Material.Dark
    Material.background: "#000000"

    Metronome {
        id: metronome
    }

    Connections {
        target: window
        function onWindowStateChanged(state) {
            if (state & Qt.WindowMaximized) {
                window.showNormal();
            }
        }
    }

    GridLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.topMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16

        columns: window.isLandscape ? 2 : 1

        BpmControl {
            id: bpmControl

            Layout.fillWidth: true
            Layout.rowSpan: window.isLandscape ? 4 : 1
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        NoteValueSelector {
            id: noteValueSelector

            Layout.minimumWidth: implicitWidth
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        Metronom {
            id: metronom

            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        Transport {
            id: transport

            Layout.minimumWidth: implicitWidth
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        AudioDeviceSelector {
            id: audioDeviceSelector

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        states: [
            State {
                name: "landscape"
                when: window.isLandscape

                PropertyChanges {
                    target: bpmControl
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                }

                PropertyChanges {
                    target: noteValueSelector
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                }

                PropertyChanges {
                    target: metronom
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                }

                PropertyChanges {
                    target: transport
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                }

                PropertyChanges {
                    target: audioDeviceSelector
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
                }
            }
        ]
    }
}
