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
    SchedulePlanner.cpp
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

target_link_libraries(rtcwake-gui PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia)

if(BUILD_DAEMON)
    add_executable(rtcwake-daemon
        DaemonMain.cpp
        RtcWakeDaemon.cpp
        RtcWakeController.cpp
        ConfigRepository.cpp
        AppConfig.cpp
        SummaryWriter.cpp
        SchedulePlanner.cpp
        ${CMAKE_SOURCE_DIR}/include/RtcWakeDaemon.h
        ${CMAKE_SOURCE_DIR}/include/RtcWakeController.h
        ${CMAKE_SOURCE_DIR}/include/ConfigRepository.h
        ${CMAKE_SOURCE_DIR}/include/AppConfig.h
        ${CMAKE_SOURCE_DIR}/include/SummaryWriter.h
        ${CMAKE_SOURCE_DIR}/include/SchedulePlanner.h
    )
    target_include_directories(rtcwake-daemon PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(rtcwake-daemon PRIVATE Qt5::Core)

    add_executable(rtcwake-warning
        WarningAppMain.cpp
        WarningBanner.cpp
        ${CMAKE_SOURCE_DIR}/include/WarningBanner.h
    )
    target_include_directories(rtcwake-warning PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(rtcwake-warning PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia)
endif()
