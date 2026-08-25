cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(WIN32)
    # MSYS2 MinGW GCC ships no libasan/libubsan runtime; even UBSan trap
    # mode (__ubsan_handle_*_abort) needs libubsan. No sanitizers on Windows.
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,undefined")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address,undefined")
endif()

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

add_subdirectory(${SRC_DIR}/utils)
add_subdirectory(${SRC_DIR}/variables)
add_subdirectory(${SRC_DIR}/executables)
add_subdirectory(${SRC_DIR}/builtin_commands)

add_executable(shell ${SRC_DIR}/main.cpp)

# builtin_commands PUBLIC-links executables/utils/variables transitively,
# so linking it alone is sufficient.
target_link_libraries(shell PRIVATE builtin_commands)

if(WIN32)
    target_link_libraries(shell PRIVATE ws2_32)
endif()
