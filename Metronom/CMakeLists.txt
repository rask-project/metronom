qt_add_library(metronom_qml STATIC)

qt_add_qml_module(metronom_qml
    URI Metronom
    SOURCES
        Metronome.h
        Metronome.cpp
        ScreenLockInhibitor.h
        ScreenLockInhibitor.cpp
        LanguageManager.h
        LanguageManager.cpp
    QML_FILES
        AudioDeviceSelector.qml
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
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)

    find_program(WAYLAND_SCANNER NAMES wayland-scanner)
    find_path(IDLE_INHIBIT_PROTOCOL_DIR
        NAMES unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
        PATHS /usr/share/wayland-protocols /usr/local/share/wayland-protocols
    )

    if(NOT WAYLAND_SCANNER OR NOT IDLE_INHIBIT_PROTOCOL_DIR)
        message(FATAL_ERROR "wayland-scanner or idle-inhibit-unstable-v1.xml not found (install wayland and wayland-protocols)")
    endif()

    set(IDLE_INHIBIT_PROTOCOL_HEADER ${CMAKE_CURRENT_BINARY_DIR}/idle-inhibit-unstable-v1-protocol.h)
    set(IDLE_INHIBIT_PROTOCOL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/idle-inhibit-unstable-v1-protocol.c)

    add_custom_command(
        OUTPUT ${IDLE_INHIBIT_PROTOCOL_HEADER} ${IDLE_INHIBIT_PROTOCOL_SOURCE}
        COMMAND ${WAYLAND_SCANNER} client-header
                ${IDLE_INHIBIT_PROTOCOL_DIR}/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
                ${IDLE_INHIBIT_PROTOCOL_HEADER}
        COMMAND ${WAYLAND_SCANNER} private-code
                ${IDLE_INHIBIT_PROTOCOL_DIR}/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
                ${IDLE_INHIBIT_PROTOCOL_SOURCE}
        DEPENDS ${IDLE_INHIBIT_PROTOCOL_DIR}/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
        VERBATIM
    )

    target_sources(metronom_qml PRIVATE ${IDLE_INHIBIT_PROTOCOL_SOURCE})
    target_include_directories(metronom_qml PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(metronom_qml PRIVATE Qt6::DBus PkgConfig::XCB PkgConfig::WAYLAND_CLIENT)
endif()

target_link_libraries(metronom_qml
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
    PRIVATE
        Qt6::Gui
        Qt6::Multimedia
)
