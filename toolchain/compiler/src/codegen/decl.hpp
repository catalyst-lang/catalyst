// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

void codegen(codegen::state &state, ast::decl_ptr decl);
void codegen(codegen::state &state, ast::decl_fn &decl);
void codegen(codegen::state &state, ast::fn_body &body);
void codegen(codegen::state &state, ast::fn_body_block &body);
void codegen(codegen::state &state, ast::fn_body_expr &body);
void codegen(codegen::state &state, ast::decl_var &decl);
void codegen(codegen::state &state, ast::decl_const &decl);
void codegen(codegen::state &state, ast::decl_struct &decl);

int locals_pass(codegen::state &state, int n, ast::decl_ptr &decl);

int proto_pass(codegen::state &state, int n, ast::decl_ptr decl);
int proto_pass(codegen::state &state, int n, ast::decl_fn &decl);
int proto_pass(codegen::state &state, int n, ast::decl_var &decl);
int proto_pass(codegen::state &state, int n, ast::decl_struct &decl);

} // namespace catalyst::compiler::codegen
