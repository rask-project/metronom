import QtQuick
import QtQuick.Window
import QtQuick.Effects

Item {
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
