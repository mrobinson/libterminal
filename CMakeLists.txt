project(ptylink)
cmake_minimum_required(VERSION 2.8)
find_package (Threads)

# SET(CMAKE_BUILD_TYPE Debug)


include_directories(src)

set(ptylink_headers
    src/pty.h
    src/VT100.h
    src/VT100Client.h
    src/SimpleVT100Client.h)

set(ptylink_sources 
    build/VT100.cpp
    src/pty.cpp
    src/SimpleVT100Client.cpp
    src/ptylink/main.cpp)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Werror -lpthread")

SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Werror -lpthread")

ADD_EXECUTABLE(ptylink ${ptylink_headers} ${ptylink_sources})
TARGET_LINK_LIBRARIES(ptylink ${CMAKE_THREAD_LIBS_INIT}) 

set(test_vt100_sources 
    build/VT100.cpp
    src/SimpleVT100Client.cpp
    src/test/VT100Harness.cpp)
ADD_EXECUTABLE(test-vt100 ${test_vt100_sources})

ADD_CUSTOM_COMMAND(
    OUTPUT VT100.cpp
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/VT100.rl
    COMMAND ragel -o ${CMAKE_CURRENT_BINARY_DIR}/VT100.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/VT100.rl
)
