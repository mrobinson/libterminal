project(ptylink)
cmake_minimum_required(VERSION 2.8)

# SET(CMAKE_BUILD_TYPE Debug)

set(ptylink_headers src/pty.h)

set(ptylink_sources src/pty.cpp src/main.cpp)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Werror")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Werror")

ADD_EXECUTABLE(ptylink ${ptylink_headers} ${ptylink_sources})
