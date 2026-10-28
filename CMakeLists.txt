cmake_minimum_required(VERSION 3.21)

project(Loali VERSION 0.6.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 WaylandCompositor DBus)

qt_standard_project_setup()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/MShapes.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

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
        qml/Desktop.qml
        qml/Launcher.qml
        qml/LockScreen.qml
        qml/SettingsApp.qml
        qml/NotificationPopups.qml
        qml/TrayMenuPanel.qml
        qml/Expo.qml
        qml/LayerChrome.qml
        qml/XWaylandChrome.qml
        qml/MShapes.qml
        qml/MShape.qml
        qml/MorphShape.qml
        qml/MaterialShapes.js
        qml/MaterialShapeData.js
        qml/WallpaperTint.qml
        qml/BlurBackdrop.qml
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
        src/xwayland.h
        src/xwayland.cpp
        src/idle.h
        src/idle.cpp
        src/datacontrol.h
        src/datacontrol.cpp
        src/sessionlock.h
        src/sessionlock.cpp
        src/inputmethod.h
        src/inputmethod.cpp
        src/blur.h
        src/blur.cpp
        src/sessionlockview.h
        src/sessionlockview.cpp
        src/wallpaperbg.h
        src/wallpaperbg.cpp
        src/portals.h
        src/portals.cpp
        src/powerservice.h
        src/powerservice.cpp
        src/util.h
    RESOURCES
        assets/logo.svg
        assets/wallpapers/dusk.jpg
        qml/blur.vert.qsb
        qml/blur.frag.qsb
)

target_sources(loali PRIVATE ${MAT_SOURCES})
target_sources(loali PRIVATE tools/lolayer_iface.c)

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
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xwayland-shell-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-idle-notify-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-output-power-management-unstable-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-data-control-unstable-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-session-lock-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/input-method-unstable-v2.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/virtual-keyboard-unstable-v1.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/blur.xml
)
qt6_generate_wayland_protocol_server_sources(loali
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-background-effect-v1.xml
)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(WAYLAND_SERVER IMPORTED_TARGET wayland-server)
    if(TARGET PkgConfig::WAYLAND_SERVER)
        target_link_libraries(loali PRIVATE PkgConfig::WAYLAND_SERVER)
    endif()
    pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
    target_link_libraries(loali PRIVATE PkgConfig::XCB)
    pkg_check_modules(XCB_COMPOSITE REQUIRED IMPORTED_TARGET xcb-composite)
    target_link_libraries(loali PRIVATE PkgConfig::XCB_COMPOSITE)
    if(TARGET Qt6::WaylandCompositorPrivate)
        get_target_property(_wcp_dirs Qt6::WaylandCompositorPrivate INTERFACE_INCLUDE_DIRECTORIES)
        set(_wcp_ok TRUE)
        foreach(_p ${_wcp_dirs})
            if(NOT "${_p}" STREQUAL "_wcp_dirs-NOTFOUND")
                get_filename_component(_wcp_abs "${_p}" ABSOLUTE BASE_DIR "${QT6_INSTALL_PREFIX}")
                if(NOT EXISTS "${_wcp_abs}")
                    set(_wcp_ok FALSE)
                endif()
            endif()
        endforeach()
        if(_wcp_ok)
            target_link_libraries(loali PRIVATE Qt6::WaylandCompositorPrivate)
            set(_qti_header "")
            foreach(_p ${_wcp_dirs})
                if(NOT "${_p}" STREQUAL "_wcp_dirs-NOTFOUND")
                    get_filename_component(_p_abs "${_p}" ABSOLUTE BASE_DIR "${QT6_INSTALL_PREFIX}")
                    foreach(_sub "" "/private")
                        set(_cand "${_p_abs}${_sub}/qwayland-server-qt-text-input-method-unstable-v1.h")
                        if(EXISTS "${_cand}")
                            set(_qti_header "${_cand}")
                            break()
                        endif()
                    endforeach()
                    if(_qti_header)
                        break()
                    endif()
                endif()
            endforeach()
            if(_qti_header)
                target_compile_definitions(loali PRIVATE HAVE_QT_TEXTINPUT_BRIDGE)
                message(STATUS "qt-text-input bridge header found: ${_qti_header}")
            else()
                message(STATUS "qt-text-input bridge disabled: private header missing")
            endif()
        else()
            message(STATUS "QtWaylandCompositor private headers missing: selection bridge off")
        endif()
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
    add_executable(lolayer tools/lolayer.c tools/lolayer_iface.c ${LOALI_LAYER_CLIENT_C})
    target_include_directories(lolayer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(lolayer PRIVATE PkgConfig::WAYLAND_CLIENT)
    install(TARGETS lolayer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    set(LOALI_IDLE_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-idle-notify-v1.xml)
    set(LOALI_POWER_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-output-power-management-unstable-v1.xml)
    set(LOALI_SLOCK_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-session-lock-v1.xml)
    set(LOALI_IM_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/input-method-unstable-v2.xml)
    set(LOALI_VK_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/virtual-keyboard-unstable-v1.xml)
    set(LOALI_IM_H ${CMAKE_CURRENT_BINARY_DIR}/input-method-unstable-v2-client-protocol.h)
    set(LOALI_IM_C ${CMAKE_CURRENT_BINARY_DIR}/input-method-unstable-v2-protocol.c)
    set(LOALI_VK_H ${CMAKE_CURRENT_BINARY_DIR}/virtual-keyboard-unstable-v1-client-protocol.h)
    set(LOALI_VK_C ${CMAKE_CURRENT_BINARY_DIR}/virtual-keyboard-unstable-v1-protocol.c)
    set(LOALI_BLUR_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/blur.xml)
    set(LOALI_BGEFF_XML ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ext-background-effect-v1.xml)
    set(LOALI_XDG_H ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h)
    set(LOALI_VP_H ${CMAKE_CURRENT_BINARY_DIR}/viewporter-client-protocol.h)
    set(LOALI_VP_C ${CMAKE_CURRENT_BINARY_DIR}/viewporter-protocol.c)
    set(LOALI_XDGOUT_H ${CMAKE_CURRENT_BINARY_DIR}/xdg-output-unstable-v1-client-protocol.h)
    set(LOALI_XDGOUT_C ${CMAKE_CURRENT_BINARY_DIR}/xdg-output-unstable-v1-protocol.c)
    set(LOALI_XDG_C ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c)
    set(LOALI_BLUR_H ${CMAKE_CURRENT_BINARY_DIR}/blur-client-protocol.h)
    set(LOALI_BLUR_C ${CMAKE_CURRENT_BINARY_DIR}/blur-protocol.c)
    set(LOALI_BGEFF_H ${CMAKE_CURRENT_BINARY_DIR}/ext-background-effect-v1-client-protocol.h)
    set(LOALI_BGEFF_C ${CMAKE_CURRENT_BINARY_DIR}/ext-background-effect-v1-protocol.c)
    set(LOALI_LOTEST_H ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-client-protocol.h)
    set(LOALI_LOTEST_C ${CMAKE_CURRENT_BINARY_DIR}/ext-idle-notify-v1-protocol.c)
    set(LOALI_POWER_H ${CMAKE_CURRENT_BINARY_DIR}/wlr-output-power-management-unstable-v1-client-protocol.h)
    set(LOALI_POWER_C ${CMAKE_CURRENT_BINARY_DIR}/wlr-output-power-management-unstable-v1-protocol.c)
    set(LOALI_SLOCK_H ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-client-protocol.h)
    set(LOALI_SLOCK_C ${CMAKE_CURRENT_BINARY_DIR}/ext-session-lock-v1-protocol.c)
    add_custom_command(
        OUTPUT ${LOALI_LOTEST_H} ${LOALI_LOTEST_C} ${LOALI_POWER_H} ${LOALI_POWER_C}
               ${LOALI_SLOCK_H} ${LOALI_SLOCK_C} ${LOALI_IM_H} ${LOALI_IM_C}
               ${LOALI_VK_H} ${LOALI_VK_C} ${LOALI_BLUR_H} ${LOALI_BLUR_C}
               ${LOALI_BGEFF_H} ${LOALI_BGEFF_C} ${LOALI_XDG_H} ${LOALI_XDG_C} ${LOALI_VP_H} ${LOALI_VP_C} ${LOALI_XDGOUT_H} ${LOALI_XDGOUT_C}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_IDLE_XML} ${LOALI_LOTEST_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_POWER_XML} ${LOALI_POWER_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_SLOCK_XML} ${LOALI_SLOCK_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_IM_XML} ${LOALI_IM_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_VK_XML} ${LOALI_VK_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_BLUR_XML} ${LOALI_BLUR_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${LOALI_BGEFF_XML} ${LOALI_BGEFF_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-shell.xml ${LOALI_XDG_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/viewporter.xml ${LOALI_VP_H}
        COMMAND ${WAYLAND_SCANNER_EXE} client-header ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-output-unstable-v1.xml ${LOALI_XDGOUT_H}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_IDLE_XML} ${LOALI_LOTEST_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_POWER_XML} ${LOALI_POWER_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_SLOCK_XML} ${LOALI_SLOCK_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_IM_XML} ${LOALI_IM_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_VK_XML} ${LOALI_VK_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_BLUR_XML} ${LOALI_BLUR_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${LOALI_BGEFF_XML} ${LOALI_BGEFF_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-shell.xml ${LOALI_XDG_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/viewporter.xml ${LOALI_VP_C}
        COMMAND ${WAYLAND_SCANNER_EXE} private-code ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-output-unstable-v1.xml ${LOALI_XDGOUT_C}
        DEPENDS ${LOALI_IDLE_XML} ${LOALI_POWER_XML} ${LOALI_SLOCK_XML}
                ${LOALI_IM_XML} ${LOALI_VK_XML} ${LOALI_BLUR_XML} ${LOALI_BGEFF_XML}
                ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-shell.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/protocols/viewporter.xml
                ${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-output-unstable-v1.xml
    )
    add_executable(lotest tools/lotest.c
        ${LOALI_LOTEST_C}
        ${LOALI_POWER_C}
        ${LOALI_SLOCK_C}
        ${LOALI_IM_C}
        ${LOALI_VK_C}
        ${LOALI_BLUR_C}
        ${LOALI_BGEFF_C}
        ${LOALI_XDG_C}
        ${LOALI_VP_C}
        ${LOALI_XDGOUT_C}
    )
    target_include_directories(lotest PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(lotest PRIVATE PkgConfig::WAYLAND_CLIENT)
else()
    message(STATUS "lolayer demo client skipped: wayland-scanner or wayland-client missing")
endif()

pkg_check_modules(PIPEWIRE QUIET IMPORTED_TARGET libpipewire-0.3)
if(TARGET PkgConfig::PIPEWIRE)
    target_sources(loali PRIVATE src/pwcast.h src/pwcast.cpp)
    target_link_libraries(loali PRIVATE PkgConfig::PIPEWIRE)
    target_compile_definitions(loali PRIVATE HAVE_PIPEWIRE)
    message(STATUS "PipeWire screencast portal enabled")
else()
    message(STATUS "PipeWire missing: screencast portal disabled")
endif()

include(GNUInstallDirs)

install(TARGETS loali lodemo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/loali.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/loali-session.desktop RENAME loali.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions)
install(FILES assets/logo.svg RENAME loali.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
