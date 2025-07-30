
cmake_minimum_required(VERSION 3.13)

add_library(variables
    STATIC
    variables.cpp
    variables.hpp
)

target_include_directories(variables
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(variables
    PUBLIC
    cxx_std_23
)

target_compile_options(variables
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
