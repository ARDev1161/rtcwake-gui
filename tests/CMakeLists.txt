find_package(Qt5 REQUIRED COMPONENTS Test Core)

set(TEST_SUPPORT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/ConfigRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/AppConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/SchedulePlanner.cpp
    ${CMAKE_SOURCE_DIR}/src/RtcWakeController.cpp
    ${CMAKE_SOURCE_DIR}/src/SummaryWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/RtcWakeDaemon.cpp
)

set(TEST_SUPPORT_HEADERS
    ${CMAKE_SOURCE_DIR}/include/ConfigRepository.h
    ${CMAKE_SOURCE_DIR}/include/AppConfig.h
    ${CMAKE_SOURCE_DIR}/include/SchedulePlanner.h
    ${CMAKE_SOURCE_DIR}/include/RtcWakeController.h
    ${CMAKE_SOURCE_DIR}/include/SummaryWriter.h
    ${CMAKE_SOURCE_DIR}/include/RtcWakeDaemon.h
)

function(add_rtcwake_test TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME}
        ${SOURCE_FILE}
        ${TEST_SUPPORT_SOURCES}
        ${TEST_SUPPORT_HEADERS}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES AUTOMOC ON)
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt5::Core Qt5::Test)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()

add_rtcwake_test(rtcwake-configrepo-test ConfigRepositoryTest.cpp)
add_rtcwake_test(rtcwake-scheduleplanner-test SchedulePlannerTest.cpp)
add_rtcwake_test(rtcwake-logging-test RtcWakeLoggingTest.cpp)
