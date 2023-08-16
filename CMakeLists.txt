cmake_minimum_required(VERSION 3.12)
project(warp-lang VERSION 0.0.1 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib/term-utils)
add_subdirectory(lib/unic)
add_subdirectory(src/warp-core)
add_subdirectory(src/warp-cli)