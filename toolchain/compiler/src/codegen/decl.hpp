// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value* codegen(codegen::state &state, ast::decl_ptr decl);
llvm::Value* codegen(codegen::state &state, ast::decl_fn &decl);
llvm::Value* codegen(codegen::state &state, ast::fn_body &body);
llvm::Value* codegen(codegen::state &state, ast::fn_body_block &body);
llvm::Value* codegen(codegen::state &state, ast::fn_body_expr &body);
llvm::Value* codegen(codegen::state &state, ast::decl_var &decl);
llvm::Value* codegen(codegen::state &state, ast::decl_struct &decl);
llvm::Value* codegen(codegen::state &state, ast::decl_class &decl);
llvm::Value* codegen(codegen::state &state, ast::decl_ns &decl);

bool check_decl_classifiers(codegen::state &state, const ast::decl_fn &decl);
bool check_decl_classifiers(codegen::state &state, const ast::decl_ns &decl);
bool check_decl_classifiers(codegen::state &state, const ast::decl_class &decl);
bool check_decl_classifiers(codegen::state &state, const ast::decl_struct &decl);
bool check_decl_classifiers(codegen::state &state, const ast::decl_var &decl);

} // namespace catalyst::compiler::codegen
