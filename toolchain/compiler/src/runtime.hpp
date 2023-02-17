// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
namespace catalyst::compiler {
struct runtime;
}

#include <string>
#include <unordered_map>
#include <memory>
#include "target.hpp"
#include "codegen/codegen.hpp"
#include "llvm/ExecutionEngine/JITSymbol.h"

namespace catalyst::compiler {

struct runtime : target {
    runtime() = delete;
    runtime(runtime&) = delete;
    explicit runtime(codegen::state &state);
     ~runtime() {};
    void insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<catalyst::compiler::codegen::type> &return_type);
    void insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<catalyst::compiler::codegen::type> &return_type, std::vector<std::shared_ptr<catalyst::compiler::codegen::type>> const &parameters);
    void register_symbols() override;
    
    llvm::FunctionCallee get_malloc() override;
    llvm::FunctionCallee get_realloc() override;
    llvm::FunctionCallee get_free() override;

    private:
    std::unordered_map<std::string, llvm::JITTargetAddress> functions;

    template <typename T> 
    friend T run_jit(codegen::state &state, const std::string &); 
};

} // namespace catalyst::compiler
