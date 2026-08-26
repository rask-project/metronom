qt_add_library(metronom_qml STATIC)

qt_add_qml_module(metronom_qml
    URI Metronom
    SOURCES
        Metronome.h
        Metronome.cpp
    QML_FILES
        AudioDeviceSelector.qml
        BpmControl.qml
        Metronom.qml
        NoteValueSelector.qml
        Transport.qml
)

target_link_libraries(metronom_qml
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
    PRIVATE
        Qt6::Multimedia
)
