cmake_minimum_required(VERSION 3.14)
project(catalyst-parser VERSION 0.0.1 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(lexy URL https://lexy.foonathan.net/download/lexy-src.zip)
FetchContent_MakeAvailable(lexy)

add_library(libparser src/ast.hpp src/grammar.hpp src/parser.cpp src/types.hpp)
target_link_libraries(libparser PUBLIC foonathan::lexy)
target_include_directories(libparser PUBLIC ../common)
target_compile_features(libparser PUBLIC cxx_std_20)

add_executable(parser)
target_sources(parser PRIVATE src/main.cpp)
target_link_libraries(parser PUBLIC libparser)
include_directories(${rang_SOURCE_DIR})
include_directories(${cli11_SOURCE_DIR})

add_executable(test-parser tests/doctest_main.cpp tests/functions.cpp)
target_compile_features(test-parser PRIVATE cxx_std_17)
target_link_libraries(test-parser PUBLIC libparser)
include_directories(${doctest_SOURCE_DIR})
