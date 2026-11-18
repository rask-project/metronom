import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Row {
    id: root

    required property Metronome metronome

    readonly property var noteValues: ({
            [Metronome.Half]: {
                iconName: "half-note",
                label: qsTr("Half")
            },
            [Metronome.DottedQuarter]: {
                iconName: "dotted-quarter-note",
                label: qsTr("Dotted Quarter")
            },
            [Metronome.Quarter]: {
                iconName: "quarter-note",
                label: qsTr("Quarter")
            },
            [Metronome.Eighth]: {
                iconName: "eighth-note",
                label: qsTr("Eighth")
            },
            [Metronome.Sixteenth]: {
                iconName: "sixteenth-note",
                label: qsTr("Sixteenth")
            },
            [Metronome.ThirtySecond]: {
                iconName: "thirty-second-note",
                label: qsTr("Thirty Second")
            }
        })

    spacing: 16

    Repeater {
        model: root.metronome.availableNoteValues

        NoteValueButton {
            required property int modelData

            iconName: root.noteValues[modelData].iconName
            label: root.noteValues[modelData].label
            highlighted: root.metronome.noteValue === modelData

            onClicked: root.metronome.noteValue = modelData
        }
    }

    component NoteValueButton: Column {
        id: control

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
            color: control.highlighted ? Theme.accent : palette.windowText

            font.weight: Font.Light
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
