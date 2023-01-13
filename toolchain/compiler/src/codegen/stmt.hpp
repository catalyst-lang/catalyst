// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

[[nodiscard]] std::string scope_name(const ast::statement_block &stmt);

void codegen(codegen::state &state, ast::statement_ptr stmt);
void codegen(codegen::state &state, ast::statement_decl &stmt);
void codegen(codegen::state &state, ast::statement_return &stmt);
void codegen(codegen::state &state, ast::statement_expr &stmt);
void codegen(codegen::state &state, ast::statement_if &stmt);
void codegen(codegen::state &state, ast::statement_block &stmt);
void codegen(codegen::state &state, std::vector<ast::statement_ptr> const &statements);

} // namespace catalyst::compiler::codegen
