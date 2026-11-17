qt_add_library(metronom_qml STATIC)

qt_add_qml_module(metronom_qml
    URI Metronom
    SOURCES
        Metronome.h
        Metronome.cpp
        ScreenLockInhibitor.h
        ScreenLockInhibitor.cpp
        StatusBarController.h
        StatusBarController.cpp
        LanguageManager.h
        LanguageManager.cpp
    QML_FILES
        AudioDeviceSelector.qml
        AutoIncrementControl.qml
        BpmControl.qml
        LanguageSelector.qml
        Metronom.qml
        NoteValueSelector.qml
        Transport.qml
)

if(APPLE)
    target_link_libraries(metronom_qml PRIVATE "-framework IOKit" "-framework CoreFoundation")
elseif(ANDROID)
    find_package(Qt6CorePrivate REQUIRED)
    target_link_libraries(metronom_qml PRIVATE Qt6::CorePrivate)
elseif(UNIX)
    target_link_libraries(metronom_qml PRIVATE Qt6::DBus)
endif()

target_link_libraries(metronom_qml
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
    PRIVATE
        Qt6::Gui
        Qt6::Multimedia
)
