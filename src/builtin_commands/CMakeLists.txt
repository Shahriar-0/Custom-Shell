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
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(builtin_commands
    PUBLIC
    executables
    utils
    variables
)

target_compile_features(builtin_commands
    PUBLIC
    cxx_std_23
)

target_compile_options(builtin_commands
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

