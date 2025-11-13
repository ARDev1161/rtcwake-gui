cmake_minimum_required(VERSION 3.16)
project(rtcwake_gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(BUILD_PLASMA_WIDGET "Build the optional Plasma widget" OFF)
option(BUILD_DAEMON "Build the rtcwake background daemon" ON)
option(ENABLE_DOXYGEN "Generate API documentation with Doxygen" OFF)

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

add_subdirectory(src)

if(BUILD_PLASMA_WIDGET)
    find_package(Qt5 REQUIRED COMPONENTS Quick QuickWidgets)
    find_package(KF5Plasma REQUIRED)
    add_subdirectory(plasma-widget)
endif()

if(ENABLE_DOXYGEN)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    add_custom_target(doxygen_docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
