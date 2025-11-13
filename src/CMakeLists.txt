set(UI_SOURCES
    main.cpp
    MainWindow.cpp
    AnalogClockWidget.cpp
    RtcWakeController.cpp
    PowerStateDetector.cpp
    WarningBanner.cpp
    AppConfig.cpp
    ConfigRepository.cpp
    SummaryWriter.cpp
)

set(UI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/MainWindow.h
    ${CMAKE_SOURCE_DIR}/include/AnalogClockWidget.h
    ${CMAKE_SOURCE_DIR}/include/RtcWakeController.h
    ${CMAKE_SOURCE_DIR}/include/PowerStateDetector.h
    ${CMAKE_SOURCE_DIR}/include/WarningBanner.h
    ${CMAKE_SOURCE_DIR}/include/AppConfig.h
    ${CMAKE_SOURCE_DIR}/include/ConfigRepository.h
)

add_executable(rtcwake-gui
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

target_include_directories(rtcwake-gui PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(rtcwake-gui PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

if(BUILD_DAEMON)
    add_executable(rtcwake-daemon
        DaemonMain.cpp
        RtcWakeDaemon.cpp
        RtcWakeController.cpp
        ConfigRepository.cpp
        AppConfig.cpp
        SummaryWriter.cpp
        ${CMAKE_SOURCE_DIR}/include/RtcWakeDaemon.h
        ${CMAKE_SOURCE_DIR}/include/RtcWakeController.h
        ${CMAKE_SOURCE_DIR}/include/ConfigRepository.h
        ${CMAKE_SOURCE_DIR}/include/AppConfig.h
        ${CMAKE_SOURCE_DIR}/include/SummaryWriter.h
    )
    target_include_directories(rtcwake-daemon PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(rtcwake-daemon PRIVATE Qt5::Core)
endif()
