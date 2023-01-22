// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <typeinfo>

#include "catalyst/rtti.hpp"
#include "expr.hpp"
#include "expr_type.hpp"
#include "value.hpp"
#include "llvm/IR/Value.h"

namespace catalyst::compiler::codegen {

llvm::Value *codegen(codegen::state &state, ast::expr_binary_arithmetic &expr,
                     std::shared_ptr<type> expecting_type) {
	auto lhs = codegen(state, expr.lhs);
	auto rhs = codegen(state, expr.rhs);

	if (lhs == nullptr || rhs == nullptr)
		return nullptr;

	// below only works for primitive types

	auto result_type = expr_resulting_type(state, expr);
	auto lhs_type = expr_resulting_type(state, expr.lhs);
	auto rhs_type = expr_resulting_type(state, expr.rhs);

	switch (expr.op) {
	case ast::expr_binary_arithmetic::op_t::plus:
		return lhs_type->create_add(state, *result_type, lhs, *rhs_type, rhs);
	case ast::expr_binary_arithmetic::op_t::minus:
		return lhs_type->create_sub(state, *result_type, lhs, *rhs_type, rhs);
	case ast::expr_binary_arithmetic::op_t::times:
		return lhs_type->create_mul(state, *result_type, lhs, *rhs_type, rhs);
	case ast::expr_binary_arithmetic::op_t::div:
		return lhs_type->create_div(state, *result_type, lhs, *rhs_type, rhs);
	default:
		state.report_message(report_type::error, "Operator not implemented", &expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_unary_arithmetic &expr,
                     std::shared_ptr<type> expecting_type) {
	auto rhs = codegen(state, expr.rhs);

	if (rhs == nullptr)
		return nullptr;

	switch (expr.op) {
	case ast::expr_unary_arithmetic::op_t::complement:
		return state.Builder.CreateXor(rhs, -1, "xortmp");
	case ast::expr_unary_arithmetic::op_t::negate:
		return state.Builder.CreateNeg(rhs, "negtmp");
	default:
		state.report_message(report_type::error, "Operator not implemented", &expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_logical &expr,
                     std::shared_ptr<type> expecting_type) {
	state.report_message(report_type::error, "expr_binary_logical: Not implemented", &expr);
	return nullptr;
}

} // namespace catalyst::compiler::codegen
