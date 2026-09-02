import QtQuick
import QtQuick.Window
import QtCore
import QtQuick.Controls
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
        opacity: 0.8
        border.color: Material.theme === Material.Dark ? Qt.darker(Material.accent, 10) : Qt.lighter(Material.accent, 1.8)
        border.width: 1
    }

    T.Overlay.modal: Item {
        id: overlayBackground

        readonly property Item windowContentItem: Window.window?.contentItem ?? null
        readonly property point blurOrigin: windowContentItem ? mapToItem(windowContentItem, 0, 0) : Qt.point(0, 0)

        ShaderEffectSource {
            id: overlayBlurSource

            anchors.fill: parent

            sourceItem: parent.windowContentItem
            sourceRect: Qt.rect(parent.blurOrigin.x, parent.blurOrigin.y, width, height)
            hideSource: false
            live: true
            recursive: false
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
