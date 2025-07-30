
cmake_minimum_required(VERSION 3.13)

add_library(executables
    STATIC
    executables.cpp
    executables.hpp
)

target_include_directories(executables
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(executables
    PUBLIC
    cxx_std_23
)

target_compile_options(executables
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
