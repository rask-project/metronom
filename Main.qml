import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

import Metronom

ApplicationWindow {
    id: window

    minimumWidth: 380
    minimumHeight: 600
    maximumWidth: 800
    maximumHeight: 600

    visible: true
    title: qsTr("Rask Metronom")

    readonly property bool isLandscape: width > height && width >= layout.anchors.leftMargin + layout.anchors.rightMargin + bpmControl.implicitWidth + Math.max(noteValueSelector.implicitWidth, transport.implicitWidth)

    Material.theme: Material.Dark
    Material.background: "#000000"

    Metronome {
        id: metronome
    }

    Connections {
        target: window
        function onWindowStateChanged(state) {
            if (Qt.platform.os !== "android" && (state & Qt.WindowMaximized)) {
                window.showNormal();
            }
        }
    }

    Component.onCompleted: {
        if (Qt.platform.os === "android") {
            return;
        }

        window.flags |= Qt.WindowMaximizeButtonHint;
    }

    GridLayout {
        id: layout

        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.topMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16

        columns: window.isLandscape ? 2 : 1
        uniformCellHeights: false

        NoteValueSelector {
            id: noteValueSelector

            Layout.minimumWidth: implicitWidth
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        BpmControl {
            id: bpmControl

            Layout.fillWidth: true
            Layout.minimumHeight: 250
            Layout.rowSpan: window.isLandscape ? 4 : 1
            Layout.alignment: Qt.AlignHCenter
            metronome: metronome
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Metronom {
                id: metronom

                height: parent.height * 0.7
                width: height
                anchors.centerIn: parent

                metronome: metronome
            }
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

    Shortcut {
        sequences: [StandardKey.Close, StandardKey.Quit]
        onActivated: Qt.quit()
    }
}
