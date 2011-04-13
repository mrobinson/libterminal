project(ptylink)
cmake_minimum_required(VERSION 2.8)

# SET(CMAKE_BUILD_TYPE Debug)

set(ptylink_headers src/pty.h)

set(ptylink_sources 
    build/VT100.cpp
    src/pty.cpp
    src/main.cpp)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Werror")

SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Werror")

ADD_EXECUTABLE(ptylink ${ptylink_headers} ${ptylink_sources})

set(test_vt100_sources 
    build/VT100.cpp
    src/test/VT100Harness.cpp)
ADD_EXECUTABLE(test-vt100 ${test_vt100_sources})

ADD_CUSTOM_COMMAND(
    OUTPUT VT100.cpp
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/VT100.rl
    COMMAND ragel -o ${CMAKE_CURRENT_BINARY_DIR}/VT100.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/VT100.rl
)
