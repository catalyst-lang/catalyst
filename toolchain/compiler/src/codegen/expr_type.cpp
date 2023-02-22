// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <sstream>
#include <typeinfo>

#include "catalyst/rtti.hpp"
#include "expr_type.hpp"
#include "function_overloading.hpp"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_ptr expr,
                                          std::shared_ptr<type> expecting_type) {
	if (isa<ast::expr_ident>(expr)) {
		return expr_resulting_type(state, *(ast::expr_ident *)expr.get(), expecting_type);
	} else if (isa<ast::expr_literal_numeric>(expr)) {
		return expr_resulting_type(state, *(ast::expr_literal_numeric *)expr.get(), expecting_type);
	} else if (isa<ast::expr_literal_bool>(expr)) {
		return expr_resulting_type(state, *(ast::expr_literal_bool *)expr.get(), expecting_type);
	} else if (isa<ast::expr_binary_arithmetic>(expr)) {
		return expr_resulting_type(state, *(ast::expr_binary_arithmetic *)expr.get(),
		                           expecting_type);
	} else if (isa<ast::expr_unary_arithmetic>(expr)) {
		return expr_resulting_type(state, *(ast::expr_unary_arithmetic *)expr.get(),
		                           expecting_type);
	} else if (isa<ast::expr_binary_logical>(expr)) {
		return expr_resulting_type(state, *(ast::expr_binary_logical *)expr.get(), expecting_type);
	} else if (isa<ast::expr_assignment>(expr)) {
		return expr_resulting_type(state, *(ast::expr_assignment *)expr.get(), expecting_type);
	} else if (isa<ast::expr_call>(expr)) {
		return expr_resulting_type(state, *(ast::expr_call *)expr.get(), expecting_type);
	} else if (isa<ast::expr_member_access>(expr)) {
		return expr_resulting_type(state, *(ast::expr_member_access *)expr.get(), expecting_type);
	} else if (isa<ast::expr_cast>(expr)) {
		return expr_resulting_type(state, *(ast::expr_cast *)expr.get(), expecting_type);
	}

	state.report_message(report_type::error, "Expression type unsupported", expr.get());

	return type::create_builtin();
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_literal_numeric &expr,
                                          std::shared_ptr<type> expecting_type) {
	switch (expr.classifier) {
	case ast::numeric_classifier::unsigned_:
		return type::create_builtin("u64");
	case ast::numeric_classifier::signed_:
		return type::create_builtin("i64");
	case ast::numeric_classifier::size:
		return type::create_builtin("usize");
	case ast::numeric_classifier::signed8:
		return type::create_builtin("i8");
	case ast::numeric_classifier::unsigned8:
		return type::create_builtin("u8");
	case ast::numeric_classifier::signed16:
		return type::create_builtin("i16");
	case ast::numeric_classifier::unsigned16:
		return type::create_builtin("u16");
	case ast::numeric_classifier::signed32:
		return type::create_builtin("i32");
	case ast::numeric_classifier::unsigned32:
		return type::create_builtin("u32");
	case ast::numeric_classifier::signed64:
		return type::create_builtin("i64");
	case ast::numeric_classifier::unsigned64:
		return type::create_builtin("u64");
	case ast::numeric_classifier::signed128:
		return type::create_builtin("i128");
	case ast::numeric_classifier::unsigned128:
		return type::create_builtin("u128");
	case ast::numeric_classifier::float_:
		return type::create_builtin("f64");
	case ast::numeric_classifier::float16:
		return type::create_builtin("f16");
	case ast::numeric_classifier::float32:
		return type::create_builtin("f32");
	case ast::numeric_classifier::float64:
		return type::create_builtin("f64");
	case ast::numeric_classifier::float128:
		return type::create_builtin("f128");
	case ast::numeric_classifier::float80:
		return type::create_builtin("f80");

	case ast::numeric_classifier::none:
		// fallthrough
	default:
		if (expr.fraction.has_value() || (expr.exponent.has_value() && expr.exponent < 0)) {
			// floating point type, default to double
			if (expecting_type == nullptr)
				return type::create_builtin("f64");
			if (expecting_type->get_fqn() == "f16")
				return type::create_builtin("f16");
			if (expecting_type->get_fqn() == "f32")
				return type::create_builtin("f32");
			if (expecting_type->get_fqn() == "f64")
				return type::create_builtin("f64");
			if (expecting_type->get_fqn() == "f80")
				return type::create_builtin("f80");
			if (expecting_type->get_fqn() == "f128")
				return type::create_builtin("f128");
		} else {
			// integer type, default to i64
			if (expecting_type == nullptr)
				return type::create_builtin("i64");
			if (expecting_type->get_fqn() == "i8")
				return type::create_builtin("i8");
			if (expecting_type->get_fqn() == "i16")
				return type::create_builtin("i16");
			if (expecting_type->get_fqn() == "i32")
				return type::create_builtin("i32");
			if (expecting_type->get_fqn() == "i64")
				return type::create_builtin("i64");
			if (expecting_type->get_fqn() == "i128")
				return type::create_builtin("i128");
			if (expecting_type->get_fqn() == "u8")
				return type::create_builtin("u8");
			if (expecting_type->get_fqn() == "u16")
				return type::create_builtin("u16");
			if (expecting_type->get_fqn() == "u32")
				return type::create_builtin("u32");
			if (expecting_type->get_fqn() == "u64")
				return type::create_builtin("u64");
			if (expecting_type->get_fqn() == "u128")
				return type::create_builtin("u128");
		}
		return type::create_builtin("i64");
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_literal_bool &expr,
                                          std::shared_ptr<type> expecting_type) {
	return type::create_builtin("bool");
}

std::shared_ptr<type> expr_resulting_type_this(codegen::state &state, ast::expr_ident &expr,
                                               std::shared_ptr<type> expecting_type) {
	auto fn_type = (type_function *)state.current_function_symbol->type.get();
	if (!fn_type || !fn_type->is_method()) {
		return nullptr;
	}
	return std::make_shared<type_object>(fn_type->method_of);
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_ident &expr,
                                          std::shared_ptr<type> expecting_type) {
	if (expr.ident.name == "this")
		return expr_resulting_type_this(state, expr, expecting_type);

	// Look this variable up in the function.
	symbol *symbol = state.scopes.find_named_symbol(expr.ident.name);
	if (isa<type_function>(expecting_type) || symbol && isa<type_function>(symbol->type)) {
		symbol = find_function_overload(
			state, expr.ident.name, std::static_pointer_cast<type_function>(expecting_type), &expr);
	}
	if (!symbol)
		return type::create_builtin();
	return symbol->type;
}

std::shared_ptr<type> get_most_specialized_type(std::shared_ptr<type> &lhs,
                                                std::shared_ptr<type> &rhs) {
	if (lhs->specialization_score > rhs->specialization_score) {
		return lhs;
	} else if (rhs->specialization_score > lhs->specialization_score) {
		return rhs;
	} else {
		return lhs;
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_binary_arithmetic &expr,
                                          std::shared_ptr<type> expecting_type) {
	auto lhs = expr_resulting_type(state, expr.lhs);
	auto rhs = expr_resulting_type(state, expr.rhs);

	if (!lhs->is_valid() || !rhs->is_valid())
		return type::create_builtin();

	auto result_type = get_most_specialized_type(lhs, rhs);

	switch (expr.op) {
	case ast::expr_binary_arithmetic::op_t::plus:
	case ast::expr_binary_arithmetic::op_t::minus:
	case ast::expr_binary_arithmetic::op_t::times:
	case ast::expr_binary_arithmetic::op_t::div:
		return result_type;
	default:
		return type::create_builtin();
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_unary_arithmetic &expr,
                                          std::shared_ptr<type> expecting_type) {
	auto rhs = expr_resulting_type(state, expr.rhs);

	if (!rhs->is_valid())
		return type::create_builtin();

	switch (expr.op) {
	case ast::expr_unary_arithmetic::op_t::complement:
	case ast::expr_unary_arithmetic::op_t::negate:
		return type::create_builtin("bool");
	default:
		return type::create_builtin();
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_binary_logical &expr,
                                          std::shared_ptr<type> expecting_type) {
	return type::create_builtin("bool");
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_call &expr,
                                          std::shared_ptr<type> expecting_type) {
	if (isa<ast::expr_ident>(expr.lhs)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		auto sym = find_function_overload(state, callee.ident.name, expr, expecting_type);
		if (sym == nullptr) {
			return type::create_builtin();
		}
		if (!sym->type->is_valid())
			return type::create_builtin();
		if (isa<type_function>(sym->type)) {
			auto type = (type_function *)sym->type.get();
			return type->return_type;
		} else if (isa<type_custom>(sym->type)) {
			// constructor
			return std::make_shared<type_object>(std::dynamic_pointer_cast<type_custom>(sym->type));
		}
	}
	return type::create_builtin();
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_member_access &expr,
                                          std::shared_ptr<type> expecting_type) {
	auto lhs_type = expr_resulting_type(state, expr.lhs);

	if (!isa<type_object>(lhs_type)) {
		return type::create_builtin();
	}

	auto lhs_object = (type_object *)lhs_type.get();
	auto lhs_custom = lhs_object->object_type;

	if (isa<ast::expr_ident>(expr.rhs)) {
		auto ident = &((ast::expr_ident *)expr.rhs.get())->ident;
		auto member_loc = lhs_custom->get_member(ident->name);
		if (!member_loc.is_valid())
			return type::create_builtin();

		return member_loc.member->type;
	} else if (isa<ast::expr_call>(expr.rhs)) {
		auto call = (ast::expr_call *)expr.rhs.get();

		if (!isa<ast::expr_ident>(call->lhs)) {
			return type::create_builtin();
		}

		auto &ident = ((ast::expr_ident *)call->lhs.get())->ident;
		auto member_loc = lhs_custom->get_member(ident.name);
		if (isa<ast::decl_fn>(member_loc.member->decl)) {
			symbol *sym = find_function_overload(
				state, member_loc.residence->name + "." + member_loc.member->name, *call,
				expecting_type);
			if (sym == nullptr)
				return type::create_builtin();
			if (!sym->type->is_valid())
				return type::create_builtin();
			if (isa<type_function>(sym->type)) {
				auto type = (type_function *)sym->type.get();
				return type->return_type;
			} else if (isa<type_custom>(sym->type)) {
				// constructor
				return std::make_shared<type_object>(
					std::dynamic_pointer_cast<type_custom>(sym->type));
			} else {
				return type::create_builtin();
			}
		} else {
			if (!isa<type_function>(member_loc.member->type)) {
				return type::create_builtin();
			} else {
				return ((type_function *)member_loc.member->type.get())->return_type;
			}
		}

	} else {
		return type::create_builtin();
	}
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_assignment &expr,
                                          std::shared_ptr<type> expecting_type) {
	return expr_resulting_type(state, expr.lhs);
}

std::shared_ptr<type> expr_resulting_type(codegen::state &state, ast::expr_cast &expr,
                                          std::shared_ptr<type> expecting_type) {
	return type::create(state, expr.type);
}

} // namespace catalyst::compiler::codegen
