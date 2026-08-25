cmake_minimum_required(VERSION 3.16)

project(Metronom VERSION 0.1 LANGUAGES CXX)

set(TARGET_NAME metronom)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Multimedia QmlIntegration)

qt_standard_project_setup(REQUIRES 6.10)

add_subdirectory(Metronom)

qt_add_executable(${TARGET_NAME}
    main.cpp
)

qt_add_qml_module(${TARGET_NAME}
    URI App
    QML_FILES
        Main.qml
    IMPORTS TARGET metronom_qml
)

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
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        Qt6::Quick
        Qt6::Multimedia
        metronom_qml
)

install(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
