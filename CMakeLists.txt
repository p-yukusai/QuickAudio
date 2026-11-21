cmake_minimum_required(VERSION 4.4)
project(QuickAudio)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

qt_add_executable(QuickAudio main.cpp src/imm_audio.h QuickAudio.rc
        src/main_window.h)
set_target_properties(QuickAudio PROPERTIES WIN32_EXECUTABLE TRUE)

qt_add_resources(QuickAudio "theme_resources"
        PREFIX "/"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES res/icon.png)

target_link_libraries(QuickAudio PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Winmm.lib
        Ole32.lib
)

# Release builds stage everything installer.iss packages into installer-stage/.
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    get_filename_component(QT_BIN_DIR "${Qt6Core_DIR}/../../../bin" ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}" REQUIRED)

    set(VC_REDIST_PATH "" CACHE FILEPATH "Path to vc_redist.x64.exe for the installer stage")
    if (NOT VC_REDIST_PATH)
        set(_redist_candidates)
        if (DEFINED ENV{VCToolsRedistDir})
            list(APPEND _redist_candidates "$ENV{VCToolsRedistDir}/vc_redist.x64.exe")
        endif ()
        string(FIND "${CMAKE_CXX_COMPILER}" "/Tools/MSVC/" _vc_tools_pos)
        if (_vc_tools_pos GREATER -1)
            string(SUBSTRING "${CMAKE_CXX_COMPILER}" 0 ${_vc_tools_pos} _vc_dir)
            file(GLOB _redist_glob "${_vc_dir}/Redist/MSVC/*/vc_redist.x64.exe")
            list(SORT _redist_glob COMPARE NATURAL ORDER DESCENDING)
            list(APPEND _redist_candidates ${_redist_glob})
        endif ()
        foreach (_candidate IN LISTS _redist_candidates)
            if (EXISTS "${_candidate}")
                set(VC_REDIST_PATH "${_candidate}" CACHE FILEPATH "" FORCE)
                break()
            endif ()
        endforeach ()
    endif ()

    set(INSTALLER_STAGE_DIR "${CMAKE_BINARY_DIR}/installer-stage")
    add_custom_command(TARGET QuickAudio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${INSTALLER_STAGE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${INSTALLER_STAGE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:QuickAudio>" "${INSTALLER_STAGE_DIR}"
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --release --no-translations
                    --dir "${INSTALLER_STAGE_DIR}" "${INSTALLER_STAGE_DIR}/$<TARGET_FILE_NAME:QuickAudio>"
            COMMENT "Staging installer files with windeployqt"
            VERBATIM)
    if (VC_REDIST_PATH)
        add_custom_command(TARGET QuickAudio POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${VC_REDIST_PATH}" "${INSTALLER_STAGE_DIR}/vc_redist.x64.exe"
                VERBATIM)
    else ()
        message(WARNING "vc_redist.x64.exe not found; set VC_REDIST_PATH or installer.iss will fail to compile.")
    endif ()
endif ()

include(CTest)
if (BUILD_TESTING)
    add_executable(volume_dial_test tests/volume_dial_test.cpp)
    target_link_libraries(volume_dial_test PRIVATE Qt::Widgets)
    add_test(NAME volume_dial_test COMMAND volume_dial_test)
    set_tests_properties(volume_dial_test PROPERTIES ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${Qt6Core_DIR}/../../../plugins/platforms")
    add_executable(frameless_window_test tests/frameless_window_test.cpp)
    target_link_libraries(frameless_window_test PRIVATE Qt::Widgets)
    add_test(NAME frameless_window_test COMMAND frameless_window_test)
    set_tests_properties(frameless_window_test PROPERTIES ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${Qt6Core_DIR}/../../../plugins/platforms")
endif ()

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    get_filename_component(QT_INSTALL_PATH "${Qt6Core_DIR}/../../.." ABSOLUTE)
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
