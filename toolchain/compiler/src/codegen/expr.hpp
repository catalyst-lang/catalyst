// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value * codegen(codegen::state &state, ast::expr_ptr expr);
llvm::Value * codegen(codegen::state &state, ast::expr_literal_numeric &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_literal_bool &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_ident &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_binary_arithmetic &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_unary_arithmetic &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_binary_logical &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_assignment &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_call &expr);
llvm::Value * codegen(codegen::state &state, ast::expr_member_access &expr);

void codegen_assignment(codegen::state &state, llvm::Value* dest_ptr,
                        std::shared_ptr<type> dest_type, ast::expr_ptr rhs);

} // namespace catalyst::compiler::codegen
