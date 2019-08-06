cmake_minimum_required(VERSION 3.12)
project(xnet C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g0")
set(CMAKE_BUILD_TYPE Debug)


add_library(base_net-static STATIC base_net.c base_net.h)
add_library(base_net        SHARED base_net.c base_net.h)

add_library(zbytes-static STATIC zbytes.c zbytes.h packet.h packet.c)
add_library(zbytes        SHARED zbytes.c zbytes.h packet.h packet.c)
add_executable(xnet_main main.c base_net.c base_net.h)
