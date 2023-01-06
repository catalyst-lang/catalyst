// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "value.hpp"
#include "expr.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

// get an lvalue
// see https://en.cppreference.com/w/cpp/language/value_category
// in the Catalyst compiler, an lvalue is an llvm::Value address to something.
llvm::Value *get_lvalue(codegen::state &state, ast::expr_ptr expr) {
	if (typeid(*expr) == typeid(ast::expr_ident)) {
		auto expr_ident = (ast::expr_ident *)expr.get();
		auto sym = state.scopes.find_named_symbol(expr_ident->ident.name);
		if (sym == nullptr) {
			state.report_message(report_type::error, "Undefined variable name", *expr_ident);
			return nullptr;
		}
		return sym->value;
	} else if (typeid(*expr) == typeid(ast::expr_member_access)) {
		auto expr_ma = (ast::expr_member_access *)expr.get();

		auto lhs_value = codegen(state, expr_ma->lhs);
		auto lhs_type = expr_resulting_type(state, expr_ma->lhs);
		auto lhs_object = (type_object *)lhs_type.get();
		auto lhs_struct = lhs_object->object_type;

		auto ident = &((ast::expr_ident *)expr_ma->rhs.get())->ident;

		int index =
			std::distance(std::begin(lhs_struct->members), lhs_struct->members.find(ident->name));

		auto ptr =
			state.Builder.CreateStructGEP(lhs_struct->get_llvm_type(state), lhs_value, index);

		return ptr;
	}

	return nullptr;
}

} // namespace catalyst::compiler::codegen
