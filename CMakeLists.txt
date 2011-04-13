project(ptylink)
cmake_minimum_required(VERSION 2.8)

# SET(CMAKE_BUILD_TYPE Debug)

set(ptylink_headers src/pty.h)

set(ptylink_sources 
    build/vt100.cpp
    src/pty.cpp
    src/main.cpp)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Werror")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Werror")

ADD_EXECUTABLE(ptylink ${ptylink_headers} ${ptylink_sources})

ADD_CUSTOM_COMMAND(
    OUTPUT vt100.cpp
    COMMAND ragel -o ${CMAKE_CURRENT_BINARY_DIR}/vt100.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/vt100.rl
)
