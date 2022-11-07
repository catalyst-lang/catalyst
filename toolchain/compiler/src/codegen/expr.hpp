// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value *codegen(codegen::state &state, ast::expr &expr);
llvm::Value *codegen(codegen::state &state, ast::expr_literal &expr);
llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr);
llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr);
llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr);

} // namespace catalyst::compiler::codegen
