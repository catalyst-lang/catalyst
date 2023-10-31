// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "llvm/ExecutionEngine/JITSymbol.h"

#include "codegen/codegen.hpp"

namespace catalyst::compiler {

struct target {
    explicit inline target(codegen::state &state) : state(&state) {}
    virtual inline ~target() {};

    virtual inline void register_symbols() {};
    
    virtual llvm::FunctionCallee get_malloc() = 0;
    virtual llvm::FunctionCallee get_realloc() = 0;
    virtual llvm::FunctionCallee get_free() = 0;

    protected:
    codegen::state *state = nullptr;
};

} // namespace catalyst::compiler
