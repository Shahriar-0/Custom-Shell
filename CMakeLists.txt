cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

set(EXECUTABLE_DIR "${SRC_DIR}/executables")
set(UTILS_DIR "${SRC_DIR}/utils")
set(VARIABLES_DIR "${SRC_DIR}/variables")
set(BUILTIN_COMMANDS_DIR "${SRC_DIR}/builtin_commands")

add_library(executables STATIC ${EXECUTABLE_DIR}/executables.cpp ${EXECUTABLE_DIR}/executables.hpp)
add_library(utils STATIC ${UTILS_DIR}/utils.cpp ${UTILS_DIR}/utils.hpp)
add_library(variables STATIC ${VARIABLES_DIR}/variables.cpp ${VARIABLES_DIR}/variables.hpp)
add_library(builtin_commands STATIC ${BUILTIN_COMMANDS_DIR}/builtin_commands.cpp ${BUILTIN_COMMANDS_DIR}/builtin_commands.hpp)

target_include_directories(executables PUBLIC ${EXECUTABLE_DIR} PRIVATE ${SRC_DIR})
target_include_directories(utils PUBLIC ${UTILS_DIR} PRIVATE ${SRC_DIR})
target_include_directories(variables PUBLIC ${VARIABLES_DIR} PRIVATE ${SRC_DIR})
target_include_directories(builtin_commands PUBLIC ${BUILTIN_COMMANDS_DIR} PRIVATE ${SRC_DIR})

add_executable(shell ${SRC_DIR}/main.cpp)

target_link_libraries(shell PRIVATE executables utils variables builtin_commands)

if(WIN32)
    target_link_libraries(shell PRIVATE ws2_32)
endif()
