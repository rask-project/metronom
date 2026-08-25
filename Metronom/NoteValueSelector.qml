import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Row {
    id: root

    required property Metronome metronome

    spacing: 16

    RoundButton {
        icon.name: "quarter-note"

        highlighted: metronome.noteValue === Metronome.Quarter
        onClicked: metronome.noteValue = Metronome.Quarter
    }

    RoundButton {
        icon.name: "eighth-note"

        highlighted: metronome.noteValue === Metronome.Eighth
        onClicked: metronome.noteValue = Metronome.Eighth
    }

    RoundButton {
        icon.name: "sixteenth-note"

        highlighted: metronome.noteValue === Metronome.Sixteenth
        onClicked: metronome.noteValue = Metronome.Sixteenth
    }

    RoundButton {
        icon.name: "thirty-second-note"

        highlighted: metronome.noteValue === Metronome.ThirtySecond
        onClicked: metronome.noteValue = Metronome.ThirtySecond
    }
}
