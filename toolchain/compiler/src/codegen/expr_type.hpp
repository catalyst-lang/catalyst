// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_ptr expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_literal_numeric &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_literal_bool &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_ident &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_binary_arithmetic &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_unary_arithmetic &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_binary_logical &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_assignment &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_call &expr);
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_member_access &expr);

} // namespace catalyst::compiler::codegen
