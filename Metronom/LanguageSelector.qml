import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: root

    required property LanguageManager manager

    spacing: 12

    Label {
        text: qsTr("Language")
        opacity: 0.6
    }

    ComboBox {
        id: languageComboBox

        Layout.fillWidth: true
        Layout.preferredHeight: 44

        model: manager.languages
        currentIndex: {
            const index = manager.languageCodes.indexOf(manager.currentLanguage);
            return index >= 0 ? index : 0;
        }
        onActivated: index => manager.setLanguage(manager.languageCodes[index])

        ToolTip.text: qsTr("To change the language, you need to restart the application.")
        ToolTip.delay: 500
        ToolTip.timeout: 5000
        ToolTip.visible: manager.needsRestart
    }
}
