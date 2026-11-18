import QtQuick
import QtCore
import QtQuick.Controls.Material
import Metronom

import "Controls"

Window {
    id: splash

    readonly property bool isAndroid: Qt.platform.os === "android"

    visible: true
    width: splash.isAndroid ? null : 420
    height: splash.isAndroid ? null : 420

    color: "transparent"
    title: "Rask Metronom"

    modality: Qt.ApplicationModal
    flags: Qt.SplashScreen | Qt.FramelessWindowHint

    Material.theme: appSettings.theme

    Loader {
        id: mainLoader
        source: "Main.qml"
    }

    Settings {
        id: appSettings

        property int theme: Material.System
    }

    StatusBarController {
        lightStatusBar: appSettings.theme === Material.Light
    }

    QtObject {
        id: metronome

        property int bpm: 100
        property int currentSubdivision: 0
        property bool running: false

        signal ticked
    }

    Background {
        id: content

        anchors.fill: parent
        radius: 16

        Column {
            anchors.centerIn: parent
            spacing: 16

            Metronom {
                width: 98
                height: width
                anchors.horizontalCenter: parent.horizontalCenter
                metronome: metronome
            }

            Label {
                text: "Rask Metronom"
                font.pixelSize: 24
                font.bold: true

                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    Timer {
        id: beatTimer

        interval: 60000 / metronome.bpm
        running: metronome.running
        repeat: true

        onTriggered: metronome.ticked()
    }

    Timer {
        interval: 3000
        running: splash.visible
        repeat: false

        onTriggered: splash.exit()
    }

    NumberAnimation {
        id: fadeout

        target: content
        property: "opacity"
        to: 0
        duration: 500
        onFinished: splash.visible = false
    }

    function start() {
        metronome.running = true;
        metronome.runningChanged();
        metronome.ticked();
    }

    function exit() {
        metronome.running = false;
        metronome.runningChanged();
        mainLoader.item.show();
        fadeout.start();
    }

    Component.onCompleted: {
        start();

        if (Qt.platform.os === "android") {
            splash.flags |= Qt.ExpandedClientAreaHint;
            return;
        }
    }
}
