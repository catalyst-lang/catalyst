// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
namespace catalyst::compiler {
struct runtime;
}

#include <string>
#include <unordered_map>
#include <memory>
#include "codegen/codegen.hpp"
#include "llvm/ExecutionEngine/JITSymbol.h"

namespace catalyst::compiler {

struct runtime {
    runtime() = delete;
    runtime(runtime&) = delete;
    explicit runtime(codegen::state &state);
    void insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<catalyst::compiler::codegen::type> &return_type);
    void insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<catalyst::compiler::codegen::type> &return_type, std::vector<std::shared_ptr<catalyst::compiler::codegen::type>> const &parameters);
    void register_symbols();

    private:
    codegen::state *state = nullptr;
    std::unordered_map<std::string, llvm::JITTargetAddress> functions;

    template <typename T> 
    friend T run_jit(codegen::state &state, const std::string &); 
};

} // namespace catalyst::compiler
