cmake_minimum_required(VERSION 3.13)

set(BUILTIN_COMMANDS_SRC
    builtin_commands.cpp
    builtin_commands.hpp
)

add_library(builtin_commands
    STATIC
    ${BUILTIN_COMMANDS_SRC}
)

target_include_directories(builtin_commands
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

