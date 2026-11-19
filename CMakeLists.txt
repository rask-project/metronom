cmake_minimum_required(VERSION 3.16)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS target architecture")
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.4" CACHE STRING "Minimum supported macOS version")

project(Metronom VERSION 3.1.1 LANGUAGES CXX)
if(APPLE)
    enable_language(OBJCXX)
endif()

set(TARGET_NAME metronom)

set(METRONOM_CODESIGN_IDENTITY "" CACHE STRING
    "Developer ID used to sign the installed macOS bundle (empty: ad-hoc signature)")

if(APPLE AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist" CACHE PATH
        "Directory for the installed macOS application bundle" FORCE)
endif()

execute_process(
    COMMAND git rev-parse --short=7 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE METRONOM_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT METRONOM_GIT_COMMIT)
    set(METRONOM_GIT_COMMIT "unknown")
endif()

execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE METRONOM_DISPLAY_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(METRONOM_DISPLAY_VERSION)
    string(REGEX REPLACE "^v" "" METRONOM_DISPLAY_VERSION "${METRONOM_DISPLAY_VERSION}")
else()
    set(METRONOM_DISPLAY_VERSION ${PROJECT_VERSION})
endif()

# Numeric build that orders pre-releases before their release
# (1.2.0-alpha.2 -> 2, -beta.2 -> 102, -rc.2 -> 502, 1.2.0 -> 1000).
set(release_base ${PROJECT_VERSION})
set(release_build 1000)
if(METRONOM_DISPLAY_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(alpha|beta|rc)\\.([0-9]+))?$")
    set(release_base "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
    set(release_major ${CMAKE_MATCH_1})
    set(release_minor ${CMAKE_MATCH_2})
    set(release_patch ${CMAKE_MATCH_3})
    if(CMAKE_MATCH_5 STREQUAL "alpha")
        set(release_build ${CMAKE_MATCH_6})
    elseif(CMAKE_MATCH_5 STREQUAL "beta")
        math(EXPR release_build "100 + ${CMAKE_MATCH_6}")
    elseif(CMAKE_MATCH_5 STREQUAL "rc")
        math(EXPR release_build "500 + ${CMAKE_MATCH_6}")
    endif()
else()
    set(release_major ${PROJECT_VERSION_MAJOR})
    set(release_minor ${PROJECT_VERSION_MINOR})
    set(release_patch ${PROJECT_VERSION_PATCH})
endif()

# CFBundleVersion (macOS) and versionCode (Android) must grow with every release.
set(METRONOM_BUNDLE_VERSION "${release_base}.${release_build}")
math(EXPR METRONOM_VERSION_CODE
    "((${release_major} * 100 + ${release_minor}) * 100 + ${release_patch}) * 10000 + ${release_build}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick QuickControls2 Multimedia QmlIntegration LinguistTools)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Qt6 6.10 REQUIRED COMPONENTS DBus)
endif()

qt_standard_project_setup(REQUIRES 6.10)

# Updates on macOS (Sparkle). The release workflow adds each tag to the appcast
# (scripts/update-appcast.py). Debug builds have no feed URL and do not update themselves.
set(METRONOM_SPARKLE_PLIST_KEYS "")
if(APPLE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(default_feed_url "")
    else()
        set(default_feed_url "https://raw.githubusercontent.com/rask-project/metronom/appcast/appcast.xml")
    endif()
    set(METRONOM_UPDATE_FEED_URL "${default_feed_url}" CACHE STRING
        "Sparkle appcast URL (empty turns the updater off)")
    # Public half of the EdDSA key the release workflow signs updates with (SPARKLE_ED_PRIVATE_KEY secret);
    # the same key as Konsoll, as Sparkle recommends one key per developer.
    set(METRONOM_UPDATE_PUBLIC_KEY "hb9+LAm6J+NKJX6j3LRQL/G4iV4h1/eJOwS/lYJPDto=" CACHE STRING
        "Sparkle EdDSA public key (empty turns the updater off)")

    include(cmake/Sparkle.cmake)

    if(METRONOM_UPDATE_FEED_URL AND METRONOM_UPDATE_PUBLIC_KEY)
        # Checked daily, downloaded in the background and installed on quit, without Sparkle's first-run question.
        string(CONCAT METRONOM_SPARKLE_PLIST_KEYS
            "\t<key>SUFeedURL</key>\n"
            "\t<string>${METRONOM_UPDATE_FEED_URL}</string>\n"
            "\t<key>SUPublicEDKey</key>\n"
            "\t<string>${METRONOM_UPDATE_PUBLIC_KEY}</string>\n"
            "\t<key>SUVerifyUpdateBeforeExtraction</key>\n"
            "\t<true/>\n"
            "\t<key>SUEnableAutomaticChecks</key>\n"
            "\t<true/>\n"
            "\t<key>SUAutomaticallyUpdate</key>\n"
            "\t<true/>\n"
        )
    endif()
endif()

add_subdirectory(Metronom)
if(APPLE)
    add_subdirectory(MacStyle)
endif()

set(MACOSX_BUNDLE_ICON_FILE metronom.icns)
set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/resources/metronom.icns")
set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

qt_add_executable(${TARGET_NAME}
    main.cpp
    ${app_icon_macos}
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    METRONOM_VERSION="${METRONOM_DISPLAY_VERSION}"
    METRONOM_GIT_COMMIT="${METRONOM_GIT_COMMIT}"
)

qt_add_qml_module(${TARGET_NAME}
    URI App
    QML_FILES
        Main.qml
        Splash.qml
        Controls/AboutDialog.qml
        Controls/Background.qml
        Controls/BlurOverlay.qml
        Controls/DialogBackground.qml
        Controls/Header.qml
        Controls/SettingsDialog.qml
        Controls/TimeSignatureDialog.qml
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

# Selected at run time (main.cpp), so no QML import pulls it in.
if(APPLE)
    target_link_libraries(${TARGET_NAME} PRIVATE metronom_macstyleplugin)
endif()

add_subdirectory(resources/icons)

# Debug builds get their own id so they don't share macOS permissions and
# preferences caches with the installed app.
set(METRONOM_BUNDLE_ID com.github.rask-project.metronom)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(APPEND METRONOM_BUNDLE_ID ".debug")
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Metronom"
    MACOSX_BUNDLE_GUI_IDENTIFIER ${METRONOM_BUNDLE_ID}
    MACOSX_BUNDLE_BUNDLE_VERSION ${METRONOM_BUNDLE_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${METRONOM_DISPLAY_VERSION}
    MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 Rask Project"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
    QT_ANDROID_APP_NAME "Metronom"
    QT_ANDROID_VERSION_NAME ${METRONOM_DISPLAY_VERSION}
    QT_ANDROID_VERSION_CODE ${METRONOM_VERSION_CODE}
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
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

if(APPLE)
    # ditto keeps the framework's symlinks.
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND rm -rf "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/Sparkle.framework"
        COMMAND ditto "${SPARKLE_FRAMEWORK}" "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/Sparkle.framework"
        COMMENT "Embedding Sparkle.framework"
        VERBATIM
    )

    set(deploy_tool_options_arg)
    if(METRONOM_CODESIGN_IDENTITY)
        qt_policy(SET QTP0007 NEW)
        list(APPEND deploy_tool_options_arg "-sign-for-notarization=${METRONOM_CODESIGN_IDENTITY}")
    endif()

    qt_generate_deploy_qml_app_script(
        TARGET ${TARGET_NAME}
        OUTPUT_SCRIPT deploy_script
        DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
    )
    install(SCRIPT ${deploy_script})
    install(CODE "
        set(APP_BUNDLE \"\${CMAKE_INSTALL_PREFIX}/${TARGET_NAME}.app\")
        set(CODESIGN_IDENTITY \"${METRONOM_CODESIGN_IDENTITY}\")
        include(\"${CMAKE_CURRENT_SOURCE_DIR}/cmake/PruneBundle.cmake\")
    ")

    add_custom_target(package-macos
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
        DEPENDS ${TARGET_NAME}
        COMMENT "Deploying self-contained Metronom bundle to ${CMAKE_INSTALL_PREFIX}"
        VERBATIM
    )
elseif(NOT ANDROID)
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
    # "~" makes dpkg sort 1.2.0~beta.1 before 1.2.0
    string(REPLACE "-" "~" CPACK_DEBIAN_PACKAGE_VERSION "${METRONOM_DISPLAY_VERSION}")
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
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${METRONOM_DISPLAY_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_PACKAGING_INSTALL_PREFIX "${METRONOM_APP_PREFIX}")
    set(CPACK_GENERATOR "DEB")
    include(CPack)
endif()
