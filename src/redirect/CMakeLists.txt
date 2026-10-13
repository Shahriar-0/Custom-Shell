cmake_minimum_required(VERSION 3.13)

add_library(redirect
    STATIC
    redirect.cpp
    redirect.hpp
)

target_include_directories(redirect
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(redirect
    PUBLIC
    parser
)

target_compile_features(redirect
    PUBLIC
    cxx_std_23
)

target_compile_options(redirect
    PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
