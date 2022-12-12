// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <sstream>
#include <typeinfo>

#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_ptr expr, std::shared_ptr<type> expecting_type) {
	if (typeid(*expr) == typeid(ast::expr_ident)) {
		return expr_resulting_type(state, *(ast::expr_ident *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_literal_numeric)) {
		return expr_resulting_type(state, *(ast::expr_literal_numeric *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_literal_bool)) {
		return expr_resulting_type(state, *(ast::expr_literal_bool *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_binary_arithmetic)) {
		return expr_resulting_type(state, *(ast::expr_binary_arithmetic *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_unary_arithmetic)) {
		return expr_resulting_type(state, *(ast::expr_unary_arithmetic *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_binary_logical)) {
		return expr_resulting_type(state, *(ast::expr_binary_logical *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_assignment)) {
		return expr_resulting_type(state, *(ast::expr_assignment *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_call)) {
		return expr_resulting_type(state, *(ast::expr_call *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_member_access)) {
		return expr_resulting_type(state, *(ast::expr_member_access *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_cast)) {
		return expr_resulting_type(state, *(ast::expr_cast *)expr.get());
	}

	state.report_message(report_type::error, "Expression type unsupported", *expr);

	return type::create("");
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_literal_numeric &expr, std::shared_ptr<type> expecting_type) {
	switch (expr.classifier) {
	case ast::numeric_classifier::unsigned_: return type::create("u64");
	case ast::numeric_classifier::signed_: return type::create("i64");
	case ast::numeric_classifier::size: return type::create("usize");
	case ast::numeric_classifier::signed8: return type::create("i8");
	case ast::numeric_classifier::unsigned8: return type::create("u8");
	case ast::numeric_classifier::signed16: return type::create("i16");
	case ast::numeric_classifier::unsigned16: return type::create("u16");
	case ast::numeric_classifier::signed32: return type::create("i32");
	case ast::numeric_classifier::unsigned32: return type::create("u32");
	case ast::numeric_classifier::signed64: return type::create("i64");
	case ast::numeric_classifier::unsigned64: return type::create("u64");
	case ast::numeric_classifier::float_: return type::create("f64");
	case ast::numeric_classifier::float16: return type::create("f16");
	case ast::numeric_classifier::float32: return type::create("f32");
	case ast::numeric_classifier::float64: return type::create("f64");
	case ast::numeric_classifier::float128: return type::create("f128");
	case ast::numeric_classifier::float80: return type::create("f80");

	case ast::numeric_classifier::none:
		// fallthrough
	default:
		if (expr.fraction.has_value() || (expr.exponent.has_value() && expr.exponent < 0)) {
			// floating point type, default to double
			if (expecting_type == nullptr) return type::create("f64");
			if (expecting_type->get_fqn() == "f16") return type::create("f16");
			if (expecting_type->get_fqn() == "f32") return type::create("f32");
			if (expecting_type->get_fqn() == "f64") return type::create("f64");
			if (expecting_type->get_fqn() == "f80") return type::create("f80");
			if (expecting_type->get_fqn() == "f128") return type::create("f128");
		} else {
			// integer type, default to i64
			if (expecting_type == nullptr) return type::create("i64");
			if (expecting_type->get_fqn() == "i8") return type::create("i8");
			if (expecting_type->get_fqn() == "i16") return type::create("i16");
			if (expecting_type->get_fqn() == "i32") return type::create("i32");
			if (expecting_type->get_fqn() == "i64") return type::create("i64");
			if (expecting_type->get_fqn() == "i128") return type::create("i128");
			if (expecting_type->get_fqn() == "u8") return type::create("u8");
			if (expecting_type->get_fqn() == "u16") return type::create("u16");
			if (expecting_type->get_fqn() == "u32") return type::create("u32");
			if (expecting_type->get_fqn() == "u64") return type::create("u64");
			if (expecting_type->get_fqn() == "u128") return type::create("u128");
		}
		return type::create("i64");
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_literal_bool &expr, std::shared_ptr<type> expecting_type) {
	return type::create("bool");
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_ident &expr, std::shared_ptr<type> expecting_type) {
	// Look this variable up in the function.
	auto *symbol = state.scopes.find_named_symbol(expr.ident.name);
	if (!symbol)
		return type::create("");
	return symbol->type;
}

std::shared_ptr<type> get_most_specialized_type(std::shared_ptr<type> &lhs, std::shared_ptr<type> &rhs) {
	if (lhs->specialization_score > rhs->specialization_score) {
		return lhs;
	} else if (rhs->specialization_score > lhs->specialization_score) {
		return rhs;
	} else {
		return lhs;
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_binary_arithmetic &expr, std::shared_ptr<type> expecting_type) {
	auto lhs = expr_resulting_type(state, expr.lhs);
	auto rhs = expr_resulting_type(state, expr.rhs);

	if (!lhs->is_valid || !rhs->is_valid)
		return type::create("");

	auto result_type = get_most_specialized_type(lhs, rhs);

	switch (expr.op) {
	case ast::expr_binary_arithmetic::op_t::plus:
	case ast::expr_binary_arithmetic::op_t::minus:
	case ast::expr_binary_arithmetic::op_t::times:
	case ast::expr_binary_arithmetic::op_t::div:
		 return result_type;
	default:
		return type::create("");
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_unary_arithmetic &expr, std::shared_ptr<type> expecting_type) {
	auto rhs = expr_resulting_type(state, expr.rhs);

	if (!rhs->is_valid)
		return type::create("");

	switch (expr.op) {
	case ast::expr_unary_arithmetic::op_t::complement:
	case ast::expr_unary_arithmetic::op_t::negate:
		return type::create("bool");
	default:
		return type::create("");
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_binary_logical &expr, std::shared_ptr<type> expecting_type) {
	return type::create("bool");
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_call &expr, std::shared_ptr<type> expecting_type) {
	if (typeid(*expr.lhs) == typeid(ast::expr_ident)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		auto sym = state.scopes.find_named_symbol(callee.ident.name);
		if (sym == nullptr) {
			return type::create();
		}
		auto type = (type_function*)sym->type.get();
		return type->return_type;
	} else {
		return type::create();
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_member_access &expr, std::shared_ptr<type> expecting_type) {
	state.report_message(report_type::error, "expr_member_access: Not implemented");
	return type::create("");
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_assignment &expr, std::shared_ptr<type> expecting_type) {
	return expr_resulting_type(state, expr.lhs);
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_cast &expr, std::shared_ptr<type> expecting_type) {
	return type::create(expr.type);
}

} // namespace catalyst::compiler::codegen
