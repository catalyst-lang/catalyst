// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

symbol* find_function_overload(codegen::state &state, const std::string &name, ast::expr_call &expr, std::shared_ptr<type> expecting_type);

} // namespace catalyst::compiler::codegen
