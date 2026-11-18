import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import Metronom

Dialog {
    id: control

    required property Metronome metronome

    padding: 16
    topPadding: 6

    modal: true
    focus: true

    title: qsTr("Time signature")

    onAboutToShow: {
        numeratorTumbler.currentIndex = control.metronome.numerators.indexOf(control.metronome.numerator);
        denominatorTumbler.currentIndex = control.metronome.denominators.indexOf(control.metronome.denominator);
    }

    header: Label {
        text: control.title

        font.bold: true
        font.pixelSize: 18
        padding: 16

        elide: Label.ElideRight
        verticalAlignment: Label.AlignVCenter
    }

    contentItem: RowLayout {
        spacing: 8

        Tumbler {
            id: numeratorTumbler

            Layout.fillWidth: true
            Layout.preferredHeight: 180

            model: control.metronome.numerators
            delegate: TumblerLabel {}

            onCurrentIndexChanged: control.metronome.numerator = model[currentIndex]
        }

        Label {
            text: "/"
            font.pixelSize: 40
            opacity: 0.6
        }

        Tumbler {
            id: denominatorTumbler

            Layout.fillWidth: true
            Layout.preferredHeight: 180

            model: control.metronome.denominators
            delegate: TumblerLabel {}

            onCurrentIndexChanged: control.metronome.denominator = model[currentIndex]
        }
    }

    background: DialogBackground {}

    T.Overlay.modal: BlurOverlay {}

    component TumblerLabel: Label {
        required property var modelData
        required property int index

        text: modelData
        opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 40 * opacity
    }
}
