cmake_minimum_required(VERSION 3.16)
project(0space C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SDL3_IMAGE REQUIRED IMPORTED_TARGET sdl3-image)
pkg_check_modules(SDL3_MIXER REQUIRED IMPORTED_TARGET sdl3-mixer)

file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
)

add_executable(0space ${SOURCES})

target_include_directories(0space PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gen
)

target_link_libraries(0space PRIVATE
    PkgConfig::SDL3
    PkgConfig::SDL3_IMAGE
    PkgConfig::SDL3_MIXER
)

target_compile_options(0space PRIVATE -Wall -Wextra -Wno-unused-parameter)

target_compile_definitions(0space PRIVATE
    DEFAULT_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)
