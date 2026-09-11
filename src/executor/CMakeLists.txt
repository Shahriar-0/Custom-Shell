cmake_minimum_required(VERSION 3.13)

add_library(executor
    STATIC
    executor.cpp
    executor.hpp
)

target_include_directories(executor
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(executor
    PUBLIC
    parser
    builtin_commands
)

target_compile_features(executor
    PUBLIC
    cxx_std_23
)

target_compile_options(executor
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
