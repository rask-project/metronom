import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Row {
    id: root

    required property Metronome metronome

    spacing: 16

    NoteValueButton {
        metronome: root.metronome
        iconName: "quarter-note"
        label: qsTr("Quarter")

        highlighted: root.metronome.noteValue === Metronome.Quarter
        onClicked: root.metronome.noteValue = Metronome.Quarter
    }

    NoteValueButton {
        metronome: root.metronome
        iconName: "eighth-note"
        label: qsTr("Eighth")

        highlighted: root.metronome.noteValue === Metronome.Eighth
        onClicked: root.metronome.noteValue = Metronome.Eighth
    }

    NoteValueButton {
        metronome: root.metronome
        iconName: "sixteenth-note"
        label: qsTr("Sixteenth")

        highlighted: root.metronome.noteValue === Metronome.Sixteenth
        onClicked: root.metronome.noteValue = Metronome.Sixteenth
    }

    NoteValueButton {
        metronome: root.metronome
        iconName: "thirty-second-note"
        label: qsTr("Thirty Second")

        highlighted: root.metronome.noteValue === Metronome.ThirtySecond
        onClicked: root.metronome.noteValue = Metronome.ThirtySecond
    }

    component NoteValueButton: Column {
        id: control

        required property Metronome metronome
        required property string iconName
        required property string label
        required property bool highlighted

        signal clicked

        width: 72
        spacing: 5

        Button {
            id: noteButton

            height: 48
            icon.name: control.iconName

            anchors.horizontalCenter: parent.horizontalCenter

            flat: true
            highlighted: control.highlighted
            onClicked: control.clicked()
        }

        Label {
            id: noteLabel

            width: parent.width
            text: control.label
            color: control.highlighted ? Material.accent : Material.foreground

            font.weight: Font.Light
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
