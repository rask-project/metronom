import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQuick.Effects

import Metronom
import "Controls"

Window {
    id: window

    title: "Rask Metronom"

    readonly property bool isLandscape: width > height && width >= layout.anchors.leftMargin + layout.anchors.rightMargin + bpmControl.implicitWidth + Math.max(noteValueSelector.implicitWidth, transport.implicitWidth)
    readonly property Item blurSourceItem: backgroundPage
    readonly property bool isAndroid: Qt.platform.os === "android"

    minimumWidth: window.isAndroid ? null : 380
    minimumHeight: window.isAndroid ? null : 600
    maximumWidth: minimumWidth
    maximumHeight: minimumHeight

    flags: window.isAndroid ? Qt.ExpandedClientAreaHint : Qt.defaultWindowFlags & ~Qt.WindowMaximizeButtonHint

    onVisibleChanged: {
        if (visible) {
            return;
        }

        Qt.callLater(Qt.quit);
    }

    Material.theme: appSettings.theme

    Settings {
        id: appSettings

        property bool autoSave: true
        property int bpm: 120
        property int noteValue: 0
        property int theme: Material.System

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

    StatusBarController {
        lightStatusBar: appSettings.theme === Material.Light
    }

    Connections {
        target: window
        function onWindowStateChanged(state) {
            if (Qt.platform.os !== "android" && (state & Qt.WindowMaximized)) {
                window.showNormal();
            }
        }
    }

    Component.onDestruction: {
        if (!appSettings.autoSave) {
            return;
        }

        appSettings.bpm = metronome.bpm;
        appSettings.noteValue = metronome.noteValue;
    }

    Page {
        id: backgroundPage

        anchors.fill: parent
        background: Background {}

        header: Header {
            title: window.title

            settingsButton: ToolButton {
                icon.name: "settings"

                onClicked: settingsDialog.open()
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
                }
            ]
        }
    }

    Shortcut {
        sequences: [StandardKey.Close, StandardKey.Quit]
        onActivated: Qt.quit()
    }

    SettingsDialog {
        id: settingsDialog

        width: window.width > 450 ? 450 : window.width * 0.95

        anchors.centerIn: parent
        parent: Overlay.overlay

        settings: appSettings
        metronome: metronome
    }
}
