cmake_minimum_required(VERSION 3.21)

project(shell-starter-cpp CXX)

# Enforce C++23 standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------
# STATIC LINKING CONFIGURATION (a standalone .exe)
# -------------------------------------------------------------------
if(MSVC)
    # Statically link MSVC C/C++ Runtime (/MT instead of /MD)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(MINGW)
    # Statically link MinGW GCC runtimes & pthreads directly into the binary
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()
# -------------------------------------------------------------------

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