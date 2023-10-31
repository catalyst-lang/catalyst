// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"

#include "codegen.hpp"

namespace catalyst::compiler::codegen {

symbol *find_function_overload(codegen::state &state, const std::string &name, ast::expr_call &expr,
                               std::shared_ptr<type> expecting_return_type);

symbol *find_function_overload(codegen::state &state, const std::string &name,
                               std::shared_ptr<type_function> expecting_function_type = nullptr,
                               parser::ast_node *ast_node = nullptr);

} // namespace catalyst::compiler::codegen
