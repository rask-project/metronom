cmake_minimum_required(VERSION 3.16)

project(Metronom VERSION 1.0.0 LANGUAGES C CXX)

set(TARGET_NAME metronom)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Multimedia QmlIntegration LinguistTools)
find_package(Qt6 6.10 COMPONENTS DBus)

qt_standard_project_setup(REQUIRES 6.10)

add_subdirectory(Metronom)

qt_add_executable(${TARGET_NAME}
    main.cpp
)

qt_add_qml_module(${TARGET_NAME}
    URI App
    QML_FILES
        Main.qml
        Splash.qml
        Controls/Background.qml
        Controls/Header.qml
        Controls/SettingsDialog.qml
    IMPORTS TARGET metronom_qml
)

set_source_files_properties(resources/qtquickcontrols2.conf PROPERTIES QT_RESOURCE_ALIAS qtquickcontrols2.conf)

qt_add_resources(${TARGET_NAME} "resources"
    PREFIX "/"
    FILES
        resources/qtquickcontrols2.conf
        resources/logo.png
)

set_source_files_properties(Main.qml
    PROPERTIES QT_QML_GENERATE_ANDROID_JAVA_CLASS TRUE)

qt_import_qml_plugins(${TARGET_NAME})

add_subdirectory(resources/icons)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${TARGET_NAME} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${TARGET_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
    QT_ANDROID_APP_NAME "Rask Metronom"
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        Qt6::Quick
        Qt6::Multimedia
        metronom_qml
)

qt_add_translations(${TARGET_NAME}
    TS_FILES
        translations/metronom_pt_BR.ts
        translations/metronom_es_ES.ts
        translations/metronom_fr_FR.ts
        translations/metronom_it_IT.ts
)

install(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(NOT ANDROID)
    qt_generate_deploy_qml_app_script(
        TARGET ${TARGET_NAME}
        OUTPUT_SCRIPT deploy_script
    )

    install(SCRIPT ${deploy_script})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(METRONOM_APP_PREFIX "/opt/metronom")

    configure_file(resources/metronom.desktop.in metronom.desktop @ONLY)

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    file(REMOVE ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME})
    file(CREATE_LINK ${METRONOM_APP_PREFIX}/bin/${TARGET_NAME}
        ${CMAKE_BINARY_DIR}/bin/${TARGET_NAME} SYMBOLIC)

    install(CODE "
        if(CMAKE_INSTALL_PREFIX STREQUAL \"${METRONOM_APP_PREFIX}\")
            file(INSTALL \"${CMAKE_CURRENT_BINARY_DIR}/metronom.desktop\"
                DESTINATION /usr/share/applications)
            file(INSTALL \"${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.png\"
                DESTINATION /usr/share/icons/hicolor/512x512/apps RENAME metronom.png)
            file(INSTALL \"${CMAKE_BINARY_DIR}/bin/${TARGET_NAME}\"
                DESTINATION /usr/bin)
        endif()
    ")

    set(CPACK_PACKAGE_NAME "metronom")
    set(CPACK_PACKAGE_VENDOR "Rask")
    set(CPACK_PACKAGE_CONTACT "Mauro Marssola <mauro.marssola@hotmail.com>")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/rask-project/metronom")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A cross-platform metronome built with Qt 6")
    set(CPACK_DEBIAN_PACKAGE_NAME "metronom")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mauro Marssola <mauro.marssola@hotmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libc6"
        "libstdc++6"
        "libgl1"
        "libegl1"
        "libopengl0"
        "libgbm1"
        "libdrm2"
        "libepoxy0"
        "libxkbcommon0"
        "libxkbcommon-x11-0"
        "libwayland-client0"
        "libwayland-cursor0"
        "libwayland-egl1"
        "libxcb1"
        "libxcb-cursor0"
        "libxcb-icccm4"
        "libxcb-image0"
        "libxcb-keysyms1"
        "libxcb-randr0"
        "libxcb-render-util0"
        "libxcb-shape0"
        "libxcb-shm0"
        "libxcb-sync1"
        "libxcb-xfixes0"
        "libxcb-xkb1"
        "libx11-6"
        "libx11-xcb1"
        "libxext6"
        "libxfixes3"
        "libxrender1"
        "libxcomposite1"
        "libxdamage1"
        "libxcursor1"
        "libxi6"
        "libxinerama1"
        "libxrandr2"
        "libfontconfig1"
        "libfreetype6"
        "libglib2.0-0t64"
        "libdbus-1-3"
        "libsystemd0"
        "libgssapi-krb5-2"
        "libpulse0"
        "libsndfile1"
        "libgtk-3-0t64")
    # dpkg requires commas; the CMake 3.30+ DEB generator does not convert list separators
    string(REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_PACKAGING_INSTALL_PREFIX "${METRONOM_APP_PREFIX}")
    set(CPACK_GENERATOR "DEB")
    include(CPack)
endif()
