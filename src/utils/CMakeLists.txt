
cmake_minimum_required(VERSION 3.13)

add_library(utils
    STATIC
    utils.cpp
    utils.hpp
)

target_include_directories(utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(utils
    PUBLIC
    cxx_std_23
)

target_compile_options(utils
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
