cmake_minimum_required(VERSION 3.13)

add_library(parser
    STATIC
    lexer.cpp
    lexer.hpp
    parser.cpp
    parser.hpp
    token.hpp
    ast.hpp
)

target_include_directories(parser
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(parser
    PUBLIC
    cxx_std_23
)

target_compile_options(parser
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
