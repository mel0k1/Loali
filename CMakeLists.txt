cmake_minimum_required(VERSION 3.21)

project(Loali VERSION 0.6.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 WaylandCompositor DBus)

qt_standard_project_setup()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(loali
    src/main.cpp
)

file(GLOB MAT_SOURCES CONFIGURE_DEPENDS
    src/mat/cpp/*.cc
    src/mat/cpp/*/*.cc
)
list(FILTER MAT_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(FILTER MAT_SOURCES EXCLUDE REGEX "wsmeans\\.cc$")
list(FILTER MAT_SOURCES EXCLUDE REGEX "celebi\\.cc$")

qt_add_qml_module(loali
    URI Loali
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/MIcon.qml
        qml/Shadow.qml
        qml/Wallpaper.qml
        qml/Chrome.qml
        qml/Dock.qml
        qml/Taskbar.qml
        qml/Desktop.qml
        qml/Launcher.qml
        qml/LockScreen.qml
        qml/StatusPill.qml
        qml/SettingsApp.qml
        qml/NotificationPopups.qml
        qml/TrayMenuPanel.qml
        qml/Expo.qml
        qml/LayerChrome.qml
    SOURCES
        src/apps.h
        src/apps.cpp
        src/desktopmodel.h
        src/desktopmodel.cpp
        src/iconprovider.h
        src/iconprovider.cpp
        src/layershell.h
        src/layershell.cpp
        src/settings.h
        src/settings.cpp
        src/tray.h
        src/tray.cpp
        src/dbusmenu.h
        src/dbusmenu.cpp
        src/notifications.h
        src/notifications.cpp
        src/matcolor.h
        src/matcolor.cpp
        src/screencopy.h
        src/screencopy.cpp
        src/wloutput.h
        src/wloutput.cpp
        src/xdgoutput.h
        src/xdgoutput.cpp
        src/util.h
    RESOURCES
        assets/logo.svg
        assets/wallpapers/dusk.jpg
)

target_sources(loali PRIVATE ${MAT_SOURCES})

target_link_libraries(loali PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WaylandCompositor
    Qt6::DBus
)

target_include_directories(loali PRIVATE src src/mat ${CMAKE_CURRENT_BINARY_DIR})

qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-output-unstable-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-screencopy-unstable-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml
)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(WAYLAND_SERVER IMPORTED_TARGET wayland-server)
    if(TARGET PkgConfig::WAYLAND_SERVER)
        target_link_libraries(loali PRIVATE PkgConfig::WAYLAND_SERVER)
    endif()
endif()

qt_add_executable(lodemo
    tools/lodemo.cpp
)

target_link_libraries(lodemo PRIVATE
    Qt6::Qml
    Qt6::Quick
    Qt6::Gui
)

find_program(WAYLAND_SCANNER_EXE wayland-scanner)
pkg_check_modules(WAYLAND_CLIENT IMPORTED_TARGET wayland-client)
if(WAYLAND_SCANNER_EXE AND TARGET PkgConfig::WAYLAND_CLIENT)
    set(LOALI_LAYER_SHELL_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml)
    set(LOALI_LAYER_CLIENT_H ${CMAKE_CURRENT_BINARY_DIR}/layer-shell-client-protocol.h)
    set(LOALI_LAYER_CLIENT_C ${CMAKE_CURRENT_BINARY_DIR}/layer-shell-protocol.c)
    add_custom_command(
        OUTPUT ${LOALI_LAYER_CLIENT_H} ${LOALI_LAYER_CLIENT_C}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_LAYER_SHELL_XML} ${LOALI_LAYER_CLIENT_H}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_LAYER_SHELL_XML} ${LOALI_LAYER_CLIENT_C}
        DEPENDS ${LOALI_LAYER_SHELL_XML}
    )
    add_executable(lolayer tools/lolayer.c ${LOALI_LAYER_CLIENT_C})
    target_include_directories(lolayer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(lolayer PRIVATE PkgConfig::WAYLAND_CLIENT)
    install(TARGETS lolayer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
    message(STATUS "lolayer demo client skipped: wayland-scanner or wayland-client missing")
endif()

include(GNUInstallDirs)

install(TARGETS loali lodemo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/loali.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/loali-session.desktop RENAME loali.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions)
install(FILES assets/logo.svg RENAME loali.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
