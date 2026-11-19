import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    id: root

    required property TimingAnalyzer timing

    spacing: 8

    SwitchDelegate {
        Layout.fillWidth: true

        text: qsTr("Check my timing with the microphone")
        checked: root.timing.enabled
        onToggled: root.timing.enabled = checked
    }

    Label {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16

        visible: root.timing.enabled
        wrapMode: Label.WordWrap
        opacity: 0.6
        text: root.timing.calibrated ? qsTr("Calibrated for this output and input.") : qsTr("The next play starts with a few calibration clicks on the device's speaker: keep quiet until they end.")
    }

    Button {
        Layout.alignment: Qt.AlignHCenter

        visible: root.timing.enabled && root.timing.calibrated
        text: qsTr("Calibrate again")
        flat: true

        onClicked: root.timing.recalibrate()
    }
}
