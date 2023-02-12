cmake_minimum_required(VERSION 3.24)

project(fbw-cpp-framework-test LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(MSFS_SDK "C:\\MSFS SDK")

# ZERO_LVL=0 CRITICAL_LVL=1 ERROR_LVL=2 WARN_LVL=3 INFO_LVL=4 DEBUG_LVL=5 VERBOSE=6 TRACE_LVL=7
set(LOG_LEVEL 5)
set(LOGGING "LOG_LEVEL=${LOG_LEVEL}")

add_definitions(
        -DWIN32_LEAN_AND_MEAN
        -DNOMINMAX
        -DNOGDI
        -D${LOGGING}
)

include_directories(fbw-cpp-framework-test PUBLIC
        "${MSFS_SDK}/SimConnect SDK/include"
        "${CMAKE_SOURCE_DIR}/src"
        )

link_directories(fbw-cpp-framework-test PUBLIC
        "${MSFS_SDK}/SimConnect SDK/lib"
        )

add_executable(
        fbw-cpp-framework-test
        src/main.cpp
        )

target_link_libraries(
        fbw-cpp-framework-test PRIVATE
        SimConnect
)

add_custom_command(
        TARGET fbw-cpp-framework-test
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:fbw-cpp-framework-test>/fbw-cpp-framework-test.exe" "${CMAKE_SOURCE_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$ENV{MSFS_SDK}/SimConnect SDK/lib/SimConnect.dll" "${CMAKE_SOURCE_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/src/SimConnect.cfg" "${CMAKE_SOURCE_DIR}/bin"
)

