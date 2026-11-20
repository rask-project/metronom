import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Controls.Material as M
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQuick.Effects

import Metronom
import "Controls"

Window {
    id: window

    title: "Metronom"

    readonly property bool isLandscape: width > height && layout.width >= noteValueSelector.implicitWidth + layout.columnSpacing + transport.implicitWidth
    // Portrait screens too short for a metronome above the transport put the two side by side.
    readonly property bool compactPortrait: !isLandscape && layout.height - noteValueSelector.implicitHeight - bpmControl.Layout.minimumHeight - transport.implicitHeight - 2 * layout.rowSpacing < 200
    readonly property Item blurSourceItem: backgroundPage
    readonly property bool isAndroid: Qt.platform.os === "android"
    readonly property bool dialogOpen: settingsDialog.visible || aboutDialog.visible || timeSignatureDialog.visible
    // Keeps dialogs clear of the status and navigation bars on short (landscape) screens.
    readonly property real dialogMaximumHeight: height - backgroundPage.SafeArea.margins.top - backgroundPage.SafeArea.margins.bottom - 16

    minimumWidth: window.isAndroid ? null : 380
    minimumHeight: window.isAndroid ? null : 640
    maximumWidth: minimumWidth
    maximumHeight: minimumHeight

    flags: {
        if (window.isAndroid) {
            return Qt.ExpandedClientAreaHint;
        }
        if (nativeWindow.translucent) {
            return Qt.Window | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowCloseButtonHint | Qt.WindowMinimizeButtonHint | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint;
        }
        return Qt.defaultWindowFlags & ~Qt.WindowMaximizeButtonHint;
    }
    color: nativeWindow.translucent ? "transparent" : "white"

    onVisibleChanged: {
        if (visible) {
            return;
        }

        Qt.callLater(Qt.quit);
    }

    // Material only: other styles follow the color scheme set by Appearance.
    M.Material.theme: Appearance.dark ? M.Material.Dark : M.Material.Light
    palette.accent: Theme.controlAccent

    Settings {
        id: appSettings

        property bool autoSave: true
        property int bpm: 120
        property int numerator: 4
        property int denominator: 4
        property int noteValue: 0

        Component.onCompleted: {
            if (!appSettings.autoSave) {
                return;
            }

            metronome.bpm = appSettings.bpm;
            metronome.numerator = appSettings.numerator;
            metronome.denominator = appSettings.denominator;
            metronome.noteValue = appSettings.noteValue;
        }
    }

    Metronome {
        id: metronome
    }

    NativeWindow {
        id: nativeWindow

        window: window
        color: Theme.backgroundEnd
        alpha: 1.0
        dark: Theme.dark
    }

    StatusBarController {
        lightStatusBar: !Theme.dark
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
        appSettings.numerator = metronome.numerator;
        appSettings.denominator = metronome.denominator;
        appSettings.noteValue = metronome.noteValue;
    }

    Page {
        id: backgroundPage

        anchors.fill: parent
        background: Background {
            visible: !nativeWindow.translucent
        }

        header: Header {
            title: window.title
            translucent: nativeWindow.translucent
            leadingInset: nativeWindow.leadingInset

            helpButton: ToolButton {
                anchors.centerIn: parent

                icon.name: "help"

                ToolTip.text: qsTr("Help")
                ToolTip.visible: hovered
                ToolTip.delay: 500

                onClicked: Qt.openUrlExternally(AppInfo.helpUrl)
            }

            settingsButton: ToolButton {
                anchors.centerIn: parent

                icon.name: "settings"

                onClicked: settingsDialog.open()
            }
        }

        GridLayout {
            id: layout

            anchors.fill: parent
            anchors.leftMargin: 16 + parent.SafeArea.margins.left
            anchors.topMargin: 16 + parent.SafeArea.margins.top
            anchors.rightMargin: 16 + parent.SafeArea.margins.right
            anchors.bottomMargin: 16 + parent.SafeArea.margins.bottom

            columns: window.isLandscape ? 2 : 1
            uniformCellHeights: false

            NoteValueSelector {
                id: noteValueSelector

                Layout.row: 0
                Layout.column: window.isLandscape ? 1 : 0
                Layout.minimumWidth: implicitWidth
                Layout.alignment: Qt.AlignHCenter
                metronome: metronome
            }

            BpmControl {
                id: bpmControl

                Layout.row: 1
                Layout.column: window.isLandscape ? 1 : 0
                Layout.fillWidth: true
                Layout.minimumHeight: 250
                Layout.alignment: Qt.AlignHCenter
                metronome: metronome
            }

            // The metronome above the transport; beside it on compact portrait screens.
            GridLayout {
                id: playArea

                Layout.row: window.isLandscape ? 0 : 2
                Layout.column: 0
                Layout.rowSpan: window.isLandscape ? 2 : 1
                Layout.fillWidth: true
                Layout.fillHeight: true

                columns: window.compactPortrait ? 2 : 1

                Item {
                    id: metronomArea

                    Layout.fillWidth: !window.compactPortrait
                    Layout.fillHeight: true
                    Layout.preferredWidth: playArea.width * 0.3

                    Metronom {
                        id: metronom

                        height: Math.min(parent.height * 0.9, parent.width * 0.9)
                        width: height
                        anchors.centerIn: parent

                        metronome: metronome
                    }
                }

                Transport {
                    id: transport

                    Layout.fillWidth: true
                    Layout.minimumWidth: implicitWidth
                    Layout.alignment: Qt.AlignVCenter
                    metronome: metronome

                    onTimeSignatureClicked: timeSignatureDialog.open()
                }
            }

            states: [
                State {
                    name: "landscape"
                    when: window.isLandscape

                    PropertyChanges {
                        target: playArea
                        Layout.fillWidth: false
                        Layout.preferredWidth: Math.max(layout.width * 0.4, transport.implicitWidth)
                    }

                    PropertyChanges {
                        target: bpmControl
                        Layout.maximumWidth: 400
                        Layout.fillHeight: true
                        Layout.minimumHeight: 120
                    }
                }
            ]
        }
    }

    Shortcut {
        sequence: "Space"
        enabled: !window.dialogOpen
        onActivated: metronome.toggle()
    }

    Shortcut {
        sequence: "Up"
        enabled: !window.dialogOpen
        onActivated: metronome.increaseBpm()
    }

    Shortcut {
        sequence: "Down"
        enabled: !window.dialogOpen
        onActivated: metronome.decreaseBpm()
    }

    Shortcut {
        sequence: "PgUp"
        enabled: !window.dialogOpen
        onActivated: metronome.nextTempoMarking()
    }

    Shortcut {
        sequence: "PgDown"
        enabled: !window.dialogOpen
        onActivated: metronome.previousTempoMarking()
    }

    Shortcut {
        sequences: [StandardKey.Close]
        onActivated: Qt.quit()
    }

    // English on purpose: Qt moves these into the macOS app menu by matching "About", "Settings" and "Quit".
    Action {
        id: aboutAction

        text: "About Metronom"
        onTriggered: aboutDialog.open()
    }

    Action {
        id: settingsAction

        text: "Settings…"
        shortcut: StandardKey.Preferences
        onTriggered: settingsDialog.open()
    }

    Action {
        id: quitAction

        text: "Quit Metronom"
        shortcut: StandardKey.Quit
        onTriggered: Qt.quit()
    }

    Loader {
        active: AppInfo.nativeMenuBar

        sourceComponent: MenuBar {
            Menu {
                title: Qt.application.name

                MenuItem {
                    action: aboutAction
                }

                MenuItem {
                    action: settingsAction
                }

                MenuSeparator {}

                MenuItem {
                    action: quitAction
                }
            }
        }
    }

    SettingsDialog {
        id: settingsDialog

        width: window.width > 450 ? 450 : window.width * 0.95
        height: Math.min(implicitHeight, window.dialogMaximumHeight)

        anchors.centerIn: parent
        parent: Overlay.overlay

        settings: appSettings
        metronome: metronome

        onAboutRequested: {
            settingsDialog.close();
            aboutDialog.open();
        }
    }

    TimeSignatureDialog {
        id: timeSignatureDialog

        width: window.width > 450 ? 450 : window.width * 0.95
        height: Math.min(implicitHeight, window.dialogMaximumHeight)

        anchors.centerIn: parent
        parent: Overlay.overlay

        metronome: metronome
    }

    AboutDialog {
        id: aboutDialog

        width: window.width > 450 ? 450 : window.width * 0.95
        height: Math.min(implicitHeight, window.dialogMaximumHeight)

        anchors.centerIn: parent
        parent: Overlay.overlay
    }
}
