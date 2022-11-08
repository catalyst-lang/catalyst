// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value *codegen(codegen::state &state, ast::statement &stmt);
llvm::Value *codegen(codegen::state &state, ast::statement_var &stmt);
llvm::Value *codegen(codegen::state &state, ast::statement_const &stmt);
llvm::Value *codegen(codegen::state &state, ast::statement_return &stmt);
llvm::Value *codegen(codegen::state &state, ast::statement_expr &stmt);

} // namespace catalyst::compiler::codegen
