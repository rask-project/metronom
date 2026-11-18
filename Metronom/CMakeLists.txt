qt_add_library(metronom_qml STATIC)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

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
        NativeWindow.h
        NativeWindow.cpp
        AppInfo.h
        AppInfo.cpp
        Appearance.h
        Appearance.cpp
    QML_FILES
        AudioDeviceSelector.qml
        AutoIncrementControl.qml
        BpmControl.qml
        LanguageSelector.qml
        Metronom.qml
        NoteValueSelector.qml
        Theme.qml
        Transport.qml
)

if(APPLE)
    target_sources(metronom_qml PRIVATE MacWindowEffects.h MacWindowEffects.mm MacUpdater.h MacUpdater.mm)
    set_source_files_properties(MacWindowEffects.mm MacUpdater.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    target_link_libraries(metronom_qml PRIVATE
        "-framework IOKit" "-framework CoreFoundation" "-framework AppKit")
    # Sparkle.framework is embedded in the bundle's Frameworks directory (top-level CMakeLists.txt).
    get_filename_component(sparkle_framework_dir "${SPARKLE_FRAMEWORK}" DIRECTORY)
    # -iframework: a system framework path, so its headers raise no warnings.
    target_compile_options(metronom_qml PRIVATE "-iframework${sparkle_framework_dir}")
    target_link_options(metronom_qml PUBLIC "-F${sparkle_framework_dir}")
    target_link_libraries(metronom_qml PUBLIC "-framework Sparkle")
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
