set(SOURCES
    main.cpp
    MainWindow.cpp
    AnalogClockWidget.cpp
    RtcWakeController.cpp
    PowerStateDetector.cpp
    WarningBanner.cpp
)

set(HEADERS
    ${CMAKE_SOURCE_DIR}/include/MainWindow.h
    ${CMAKE_SOURCE_DIR}/include/AnalogClockWidget.h
    ${CMAKE_SOURCE_DIR}/include/RtcWakeController.h
    ${CMAKE_SOURCE_DIR}/include/PowerStateDetector.h
    ${CMAKE_SOURCE_DIR}/include/WarningBanner.h
)

add_executable(rtcwake-gui
    ${SOURCES}
    ${HEADERS}
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

target_include_directories(rtcwake-gui PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(rtcwake-gui PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
