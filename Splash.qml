import QtQuick
import QtQuick.Controls
import Metronom

import "Controls"

Window {
    id: splash

    visible: true
    color: "transparent"
    title: qsTr("Rask Metronom")

    modality: Qt.ApplicationModal
    flags: Qt.SplashScreen | Qt.FramelessWindowHint

    Loader {
        id: mainLoader
        source: "Main.qml"
    }

    QtObject {
        id: metronome

        property int bpm: 100
        property int noteValue: Metronome.Quarter
        property int currentBeat: 0
        property bool running: false
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

        onTriggered: {
            metronome.currentBeat = (metronome.currentBeat + 1) % 4;
            metronome.currentBeatChanged();
        }
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
        metronome.currentBeatChanged();
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
            return;
        }

        splash.width = 420;
        splash.height = 420;
    }
}
