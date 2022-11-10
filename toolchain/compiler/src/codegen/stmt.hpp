// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

void codegen(codegen::state &state, ast::statement_ptr stmt);
void codegen(codegen::state &state, ast::statement_var &stmt);
void codegen(codegen::state &state, ast::statement_const &stmt);
void codegen(codegen::state &state, ast::statement_return &stmt);
void codegen(codegen::state &state, ast::statement_expr &stmt);
void codegen(codegen::state &state, ast::statement_if &stmt);
void codegen(codegen::state &state, ast::statement_block &stmt);

} // namespace catalyst::compiler::codegen
