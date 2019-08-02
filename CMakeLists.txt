cmake_minimum_required(VERSION 3.12)
project(xnet C)

set(CMAKE_C_STANDARD 99)

add_library(xnet library.c library.h)

add_executable(xnet_main main.c library.c library.h)
