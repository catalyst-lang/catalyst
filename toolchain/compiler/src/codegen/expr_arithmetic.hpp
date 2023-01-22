// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value * codegen(codegen::state &state, ast::expr_binary_arithmetic &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_unary_arithmetic &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_binary_logical &expr, std::shared_ptr<type> expecting_type = nullptr);

} // namespace catalyst::compiler::codegen
