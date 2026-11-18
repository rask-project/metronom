# Qt Quick Controls style for macOS: the macOS style, plus the controls it lacks (it falls back to Fusion) drawn to
# match it. Selected in main.cpp.
qt_add_library(metronom_macstyle STATIC)

qt_add_qml_module(metronom_macstyle
    URI MacStyle
    QML_FILES
        Button.qml
        ItemDelegate.qml
        RoundButton.qml
        SwitchDelegate.qml
        ToolButton.qml
        ToolTip.qml
        Tumbler.qml
    IMPORTS
        QtQuick.Controls.macOS
)

target_link_libraries(metronom_macstyle PRIVATE Qt6::Quick)
