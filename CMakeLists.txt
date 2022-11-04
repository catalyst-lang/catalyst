cmake_minimum_required(VERSION 3.18)
project(catalyst VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TOOLCHAIN "Enable building toolchain" ON)

if(BUILD_TOOLCHAIN)
    add_subdirectory(toolchain)
endif()
