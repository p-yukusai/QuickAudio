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

qt_add_executable(QuickAudio main.cpp src/imm_audio.h
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
