// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"
#include "expr_arithmetic.hpp"

namespace catalyst::compiler::codegen {

llvm::Value * codegen(codegen::state &state, ast::expr_ptr expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_literal_numeric &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_literal_bool &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_ident &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_assignment &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_cast &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_call &expr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value * codegen(codegen::state &state, ast::expr_member_access &expr, std::shared_ptr<type> expecting_type = nullptr);

void codegen_assignment(codegen::state &state, llvm::Value* dest_ptr,
                        std::shared_ptr<type> dest_type, ast::expr_ptr rhs);

llvm::Value *codegen_call(codegen::state &state, std::shared_ptr<type> fn_type, llvm::Value *value, ast::expr_call &expr, llvm::Value *this_ = nullptr, std::shared_ptr<type> expecting_type = nullptr);
llvm::Value *codegen_call(codegen::state &state, symbol *sym, ast::expr_call &expr, llvm::Value *this_ = nullptr, std::shared_ptr<type> expecting_type = nullptr);

} // namespace catalyst::compiler::codegen
