cmake_minimum_required(VERSION 3.12)
project(xnet C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g0")
set(CMAKE_BUILD_TYPE Debug)


add_library(xnet-static STATIC library.c library.h)
add_library(xnet        SHARED library.c library.h)

add_executable(xnet_main main.c library.c library.h)
