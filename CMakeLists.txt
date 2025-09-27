cmake_minimum_required(VERSION 3.16)

project(CKFont LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_TEST_CKFONT "Enable cktext test target." OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Lz4++ REQUIRED)

add_library(ckfont STATIC
    font.h
    font.cpp
    fnt_adapter.h fnt_adapter.cpp
    drawer.h drawer.cpp
    drawer_sprite.h drawer_sprite.cpp
)
target_include_directories(ckfont PUBLIC
    . 3rd/include
)
target_link_libraries(ckfont PUBLIC Lz4++::static)

if(ENABLE_TEST_CKFONT)
    add_executable(test_ckfont main.cpp)
    target_link_libraries(test_ckfont PRIVATE ckfont)
endif()
