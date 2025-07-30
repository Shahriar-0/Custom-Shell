cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

add_subdirectory(${SRC_DIR}/executables)
add_subdirectory(${SRC_DIR}/utils)
add_subdirectory(${SRC_DIR}/variables)
add_subdirectory(${SRC_DIR}/builtin_commands)

add_executable(shell ${SRC_DIR}/main.cpp)

target_link_libraries(shell PRIVATE executables utils variables builtin_commands)

if(WIN32)
    target_link_libraries(shell PRIVATE ws2_32)
endif()
