cmake_minimum_required(VERSION 3.16)
project(ratatoskr-gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 DBus Svg)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPULSE REQUIRED libpulse)
qt_standard_project_setup()

qt_add_executable(ratatoskr-gui
    src/main.cpp
)

qt_add_qml_module(ratatoskr-gui
    URI Ratatoskr
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/StatusBar.qml
        src/qml/VolumePanel.qml
        src/qml/StreamPanel.qml
        src/qml/EqPanel.qml
        src/qml/SettingsPanel.qml
    SOURCES
        src/deviceproxy.h src/deviceproxy.cpp
        src/pulseaudiosource.h src/pulseaudiosource.cpp
)

qt_add_resources(ratatoskr-gui "icons"
    PREFIX "/icons"
    FILES
        icons/squirrel.svg
        icons/headphones.svg
        icons/headset.svg
)

target_include_directories(ratatoskr-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBPULSE_INCLUDE_DIRS}
)

target_link_libraries(ratatoskr-gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Svg
    ${LIBPULSE_LIBRARIES}
)

install(TARGETS ratatoskr-gui DESTINATION bin)
