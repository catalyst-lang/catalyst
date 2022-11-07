cmake_minimum_required(VERSION 3.14)
project(catalyst-parser VERSION 0.0.1 LANGUAGES C CXX)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24.0)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(lexy URL https://lexy.foonathan.net/download/lexy-src.zip)
FetchContent_MakeAvailable(lexy)

add_library(libparser ../common/catalyst/ast/ast.hpp src/grammar.hpp src/parser.cpp src/types.hpp src/positional.hpp src/state.hpp)
target_link_libraries(libparser PUBLIC foonathan::lexy)
target_include_directories(libparser PRIVATE ${rang_SOURCE_DIR})
target_include_directories(libparser PUBLIC ../common)
target_compile_features(libparser PUBLIC cxx_std_20)

add_executable(parser src/main.cpp)
target_link_libraries(parser PUBLIC libparser)
target_include_directories(parser PRIVATE ${rang_SOURCE_DIR})
target_include_directories(parser PRIVATE ${cli11_SOURCE_DIR})

add_executable(test-parser tests/doctest_main.cpp tests/functions.cpp)
target_compile_features(test-parser PRIVATE cxx_std_17)
target_link_libraries(test-parser PUBLIC libparser)
target_include_directories(test-parser PRIVATE ${doctest_SOURCE_DIR})
