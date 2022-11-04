cmake_minimum_required(VERSION 3.18)
project(catalyst-toolchain VERSION 0.0.1 LANGUAGES C CXX)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24.0)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
# Prepare doctest for other targets to use
FetchContent_Declare(
        doctest
        URL https://raw.githubusercontent.com/doctest/doctest/v2.4.9/doctest/doctest.h
        DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(doctest)

# Prepare rang for other targets to use
FetchContent_Declare(
        rang
        URL https://github.com/agauniyal/rang/releases/download/v3.2/rang.hpp
        DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(rang)

# Prepare CLI11 for other targets to use
FetchContent_Declare(
        cli11
        URL https://github.com/CLIUtils/CLI11/releases/download/v2.3.1/CLI11.hpp
        DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(cli11)

option(BUILD_PARSER "Enable building the parser" ON)
option(BUILD_COMPILER "Enable building the compiler" ON)

if(BUILD_PARSER)
    add_subdirectory(parser)
endif()
if(BUILD_COMPILER)
    add_subdirectory(compiler)
endif()


# Make test executable
#add_executable(tests main.cpp)
#target_compile_features(test PRIVATE cxx_std_17)
#target_link_libraries(test PRIVATE doctest::doctest)
