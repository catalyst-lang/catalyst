// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

int overloading_functions_pass(codegen::state &state, int n, ast::decl_ptr decl);
int overloading_functions_pass(codegen::state &state, int n, ast::decl_fn &decl);
int overloading_functions_pass(codegen::state &state, int n, ast::decl_var &decl);
int overloading_functions_pass(codegen::state &state, int n, ast::decl_struct &decl);

} // namespace catalyst::compiler::codegen
