cmake_minimum_required(VERSION 3.14)
project(catalyst-compiler VERSION 0.0.1 LANGUAGES C CXX)

add_library(libcompiler src/compiler.cpp src/compiler.hpp)
target_include_directories(libcompiler PUBLIC ../common)
target_compile_features(libcompiler PUBLIC cxx_std_20)

#set(BUILD_ARCH "-m64")
#set(TARGET_64 ON)

# import LLVM
find_package(LLVM REQUIRED CONFIG)
target_include_directories(libcompiler PRIVATE ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core)
target_link_libraries(libcompiler PRIVATE libparser)
target_link_libraries(libcompiler PRIVATE ${llvm_libs})

add_executable(compiler src/main.cpp)
target_link_libraries(compiler PUBLIC libcompiler)
target_include_directories(compiler PRIVATE ${rang_SOURCE_DIR})
target_include_directories(compiler PRIVATE ${cli11_SOURCE_DIR})

add_executable(test-compiler tests/doctest_main.cpp)
target_compile_features(test-compiler PRIVATE cxx_std_17)
target_link_libraries(test-compiler PUBLIC libcompiler)
target_include_directories(test-compiler PRIVATE ${doctest_SOURCE_DIR})
