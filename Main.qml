import QtQuick
import QtCore
import QtQuick.Layouts
import QtQuick.Controls

import Metronom
import "Controls"

ApplicationWindow {
    id: window

    title: qsTr("Rask Metronom")

    readonly property bool isLandscape: width > height && width >= layout.anchors.leftMargin + layout.anchors.rightMargin + bpmControl.implicitWidth + Math.max(noteValueSelector.implicitWidth, transport.implicitWidth)

    background: Background {}

    onVisibleChanged: {
        if (visible) {
            return;
        }

        Qt.callLater(Qt.quit);
    }

    header: Header {
        title: window.title

        settingsButton: ToolButton {
            icon.name: "settings"

            onClicked: settingsDialog.open()
        }
    }

    Settings {
        id: appSettings

        property bool autoSave: true
        property int bpm: 120
        property int noteValue: 0

        Component.onCompleted: {
            if (!appSettings.autoSave) {
                return;
            }

            metronome.bpm = appSettings.bpm;
            metronome.noteValue = appSettings.noteValue;
        }
    }

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
        window.minimumWidth = 380;
        window.minimumHeight = 600;
        window.maximumWidth = 800;
        window.maximumHeight = 600;
    }

    Component.onDestruction: {
        if (!appSettings.autoSave) {
            return;
        }

        appSettings.bpm = metronome.bpm;
        appSettings.noteValue = metronome.noteValue;
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

    Dialog {
        id: settingsDialog

        width: window.width > 450 ? 450 : window.width * 0.95
        topPadding: 48
        padding: 32

        anchors.centerIn: parent
        modal: true
        focus: true

        title: qsTr("Settings")

        contentItem: ColumnLayout {
            spacing: 16

            SwitchDelegate {
                Layout.fillWidth: true

                text: qsTr("Save the BPM and note value")

                checked: appSettings.autoSave
                onCheckedChanged: appSettings.autoSave = checked
            }

            LanguageSelector {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter

                manager: LanguageManager {
                    id: languageManager
                }
            }

            AudioDeviceSelector {
                id: audioDeviceSelector

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                metronome: metronome
            }
        }

        header: Label {
            visible: parent?.parent === Overlay.overlay && settingsDialog.title

            text: settingsDialog.title

            font.bold: true
            font.pixelSize: 18
            padding: 12

            elide: Label.ElideRight
            verticalAlignment: Label.AlignVCenter
        }

        background: Background {
            Material.roundedScale: Material.LargeScale
        }
    }
}
