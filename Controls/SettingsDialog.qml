import QtCore

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Effects

import Metronom

Dialog {
    id: control

    required property Settings settings
    required property Metronome metronome

    padding: 0
    topPadding: 6
    bottomPadding: 32

    modal: true
    focus: true

    title: qsTr("Settings")

    header: Label {
        visible: parent?.parent === Overlay.overlay && control.title

        text: control.title

        font.bold: true
        font.pixelSize: 18
        padding: 16

        elide: Label.ElideRight
        verticalAlignment: Label.AlignVCenter
    }

    contentItem: ColumnLayout {
        spacing: 16

        SwitchDelegate {
            Layout.fillWidth: true

            text: qsTr("Dark mode")

            checked: Material.theme === Material.Dark
            onCheckedChanged: control.settings.theme = checked ? Material.Dark : Material.Light
        }

        SwitchDelegate {
            Layout.fillWidth: true

            text: qsTr("Save the BPM and note value")

            checked: control.settings.autoSave
            onCheckedChanged: control.settings.autoSave = checked
        }

        AutoIncrementControl {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            metronome: control.metronome
        }

        LanguageSelector {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            manager: LanguageManager {
                id: languageManager
            }
        }

        AudioDeviceSelector {
            id: audioDeviceSelector

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            metronome: control.metronome
        }
    }

    background: Background {
        Material.roundedScale: Material.LargeScale

        opacity: 0.95
        border.color: Material.theme === Material.Dark ? Qt.darker(Material.accent, 10) : Qt.lighter(Material.accent, 1.8)
        border.width: 1
    }

    T.Overlay.modal: Item {
        id: overlayBackground

        // The page, not the window's contentItem: that one holds the overlay itself.
        readonly property Item blurSourceItem: Window.window?.blurSourceItem ?? null
        readonly property point blurOrigin: blurSourceItem ? mapToItem(blurSourceItem, 0, 0) : Qt.point(0, 0)

        ShaderEffectSource {
            id: overlayBlurSource

            anchors.fill: parent

            sourceItem: parent.blurSourceItem
            sourceRect: Qt.rect(parent.blurOrigin.x, parent.blurOrigin.y, width, height)
            visible: false
            hideSource: false
            live: true
            recursive: false
        }

        // Fills what the blurred page leaves transparent (translucent window), hiding the sharp page below.
        Background {
            anchors.fill: parent
            radius: 0
            opacity: 0.9
        }

        MultiEffect {
            anchors.fill: overlayBlurSource
            source: overlayBlurSource

            blurEnabled: true
            blur: 1.0
            blurMax: 32
            autoPaddingEnabled: false
        }
    }

    T.Overlay.modeless: Rectangle {
        readonly property color backgroundColor: Material.dropShadowColor

        color: Qt.rgba(backgroundColor.r, backgroundColor.g, backgroundColor.b, 0.5)
    }
}
