cmake_minimum_required(VERSION 3.16)
project(rtcwake_gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(BUILD_PLASMA_WIDGET "Build the optional Plasma widget" OFF)
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
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
    doxygen_add_docs(doxygen_docs
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        COMMENT "Generate API documentation"
    )
endif()
