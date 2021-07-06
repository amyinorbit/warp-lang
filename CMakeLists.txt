cmake_minimum_required(VERSION 3.12)
project(warp-lang VERSION 0.0.1 LANGUAGES C)

add_subdirectory(lib/term-utils)
add_subdirectory(src/warp-core)
add_subdirectory(src/warp-cli)