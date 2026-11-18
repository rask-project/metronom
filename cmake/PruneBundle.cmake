# Install step run after Qt's deploy script: removes what macdeployqt copies
# into metronom.app but the app never loads, then signs the bundle again
# (removing files breaks the seal macdeployqt made).
#
# Expects APP_BUNDLE (path to metronom.app) and CODESIGN_IDENTITY (empty for
# an ad-hoc signature).
#
# - Quick Controls styles: QtQuick.Controls lists every style as an optional
#   import, so the QML import scanner deploys all of them. Metronom runs its
#   MacStyle (main.cpp), built on the macOS style and its native style, which
#   fall back to Fusion and Basic; Material stays for the attached theme Main.qml
#   and Splash.qml import. The others are unused.
# - QtWidgets: only the widget style plugin (styles/libqmacstyle) needs it, and
#   Metronom has no widgets.
# - tls, networkinformation: Metronom's own code makes no network connections
#   (Sparkle downloads updates with NSURLSession).
# - Sparkle.framework: its headers, and the XPC services only sandboxed apps
#   need.

set(contents "${APP_BUNDLE}/Contents")
set(remove
    "${contents}/Frameworks/QtWidgets.framework"
    "${contents}/PlugIns/styles"
    "${contents}/PlugIns/tls"
    "${contents}/PlugIns/networkinformation"
)
foreach(style IN ITEMS Universal Imagine FluentWinUI3 iOS)
    string(TOLOWER "${style}" lower)
    if(style STREQUAL "iOS")
        set(framework "IOS")
    else()
        set(framework "${style}")
    endif()
    list(APPEND remove
        "${contents}/Frameworks/QtQuickControls2${framework}.framework"
        "${contents}/Frameworks/QtQuickControls2${framework}StyleImpl.framework"
        "${contents}/PlugIns/libqtquickcontrols2${lower}styleplugin.dylib"
        "${contents}/PlugIns/libqtquickcontrols2${lower}styleimplplugin.dylib"
        "${contents}/Resources/qml/QtQuick/Controls/${style}"
    )
endforeach()

set(sparkle "${contents}/Frameworks/Sparkle.framework")
foreach(part IN ITEMS Headers PrivateHeaders Modules XPCServices)
    list(APPEND remove "${sparkle}/Versions/B/${part}" "${sparkle}/${part}")
endforeach()

foreach(path IN LISTS remove)
    if(EXISTS "${path}" OR IS_SYMLINK "${path}")
        message(STATUS "Pruning: ${path}")
        file(REMOVE_RECURSE "${path}")
    endif()
endforeach()

if(CODESIGN_IDENTITY)
    set(sign_args --options runtime --timestamp --sign "${CODESIGN_IDENTITY}")
else()
    set(sign_args --sign -)
endif()
execute_process(
    COMMAND codesign --force --deep ${sign_args} "${APP_BUNDLE}"
    RESULT_VARIABLE sign_result
)
if(NOT sign_result EQUAL 0)
    message(FATAL_ERROR "codesign failed for ${APP_BUNDLE}")
endif()
