cmake_minimum_required(VERSION 3.17)
project(hw6 C)

set(CMAKE_C_STANDARD 11)

# Add -O0 to remove optimizations when using gcc
IF (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
ENDIF (CMAKE_COMPILER_IS_GNUCC)

add_executable(hw6 main.c spectre.h)
add_executable(test test.c spectre.h)