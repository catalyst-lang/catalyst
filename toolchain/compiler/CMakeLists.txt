cmake_minimum_required(VERSION 3.14)
project(catalyst-compiler VERSION 0.0.1 LANGUAGES C CXX)

add_library(libcompiler 
    src/compiler.cpp 
    src/compiler.hpp 
    src/codegen/expr.cpp 
    src/codegen/expr.hpp 
    src/codegen/expr_arithmetic.cpp 
    src/codegen/expr_arithmetic.hpp 
    src/codegen/codegen.cpp 
    src/codegen/codegen.hpp 
    src/codegen/decl.cpp 
    src/codegen/decl_classifiers.cpp 
    src/codegen/decl.hpp 
    src/codegen/decl_type.cpp 
    src/codegen/decl_type.hpp 
    src/codegen/object/struct/decl_struct.cpp 
    src/codegen/object/struct/type_struct.cpp 
    src/codegen/object/class/decl_class.cpp 
    src/codegen/object/class/type_class.cpp 
    src/codegen/object/object_type.cpp 
    src/codegen/object/object_type.hpp 
    src/codegen/object/member.hpp 
    src/codegen/object/member.cpp 
    src/codegen/stmt.cpp 
    src/codegen/stmt.hpp 
    src/codegen/value.hpp 
    src/codegen/value.cpp 
    src/codegen/scope.cpp 
    src/codegen/scope.hpp 
    src/codegen/symbol.hpp 
    src/codegen/type.hpp 
    src/codegen/type.cpp 
    src/codegen/expr_type.cpp 
    src/codegen/expr_type.hpp 
    src/codegen/decl_overloading_pass.cpp
    src/codegen/decl_overloading_pass.hpp
    src/codegen/decl_proto_pass.cpp
    src/codegen/decl_proto_pass.hpp
    src/codegen/decl_locals_pass.cpp
    src/codegen/decl_locals_pass.hpp
    src/codegen/function_overloading.cpp
    src/codegen/function_overloading.hpp
    src/codegen/pass.hpp
    src/codegen/pass.cpp
    src/target.hpp
    src/runtime.cpp 
    src/runtime.hpp
    src/jit.cpp 
    src/jit.hpp 
)
target_include_directories(libcompiler PUBLIC ../common)
target_compile_features(libcompiler PUBLIC cxx_std_20)
#set(BUILD_ARCH "-m64")
#set(TARGET_64 ON)

# import LLVM
find_package(LLVM REQUIRED CONFIG)
target_include_directories(libcompiler PRIVATE ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core orcjit native)
target_link_libraries(libcompiler PRIVATE libparser)
target_link_libraries(libcompiler PRIVATE ${llvm_libs})

add_executable(compiler src/main.cpp ../common/catalyst/ast/stmt.hpp)
target_link_libraries(compiler PUBLIC libcompiler)
target_include_directories(compiler PRIVATE ${rang_SOURCE_DIR})
target_include_directories(compiler PRIVATE ${cli11_SOURCE_DIR})
target_compile_features(compiler PUBLIC cxx_std_20)

add_executable(test-compiler 
    tests/doctest_main.cpp
    tests/functions.cpp
    tests/arithmetics.cpp
    tests/struct.cpp
    tests/class.cpp
    tests/frontend.cpp
    tests/types.cpp
    tests/branching.cpp
    tests/scopes.cpp
    tests/inheritance.cpp
    tests/namespaces.cpp
    tests/virtuals.cpp
)
target_compile_features(test-compiler PRIVATE cxx_std_20)
target_link_libraries(test-compiler PUBLIC libcompiler)
target_include_directories(test-compiler PRIVATE ${doctest_SOURCE_DIR})
