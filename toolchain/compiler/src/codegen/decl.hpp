// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value *codegen(codegen::state &state, ast::decl_ptr decl);
llvm::Value *codegen(codegen::state &state, ast::decl_fn &decl);
llvm::BasicBlock *codegen(codegen::state &state, ast::fn_body &body);
llvm::BasicBlock *codegen(codegen::state &state, ast::fn_body_block &body);
llvm::BasicBlock *codegen(codegen::state &state, ast::fn_body_expr &body);

void proto_pass(codegen::state &state, ast::decl_ptr decl);
void proto_pass(codegen::state &state, ast::decl_fn &decl);

} // namespace catalyst::compiler::codegen
