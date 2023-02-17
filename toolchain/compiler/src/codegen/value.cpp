// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"
#include "value.hpp"
#include "expr.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

// get an lvalue
// see https://en.cppreference.com/w/cpp/language/value_category
// in the Catalyst compiler, an lvalue is an llvm::Value address to something.
llvm::Value *get_lvalue(codegen::state &state, ast::expr_ptr expr) {
	if (isa<ast::expr_ident>(expr)) {
		auto expr_ident = (ast::expr_ident *)expr.get();
		auto sym = state.scopes.find_named_symbol(expr_ident->ident.name);
		if (sym == nullptr) {
			state.report_message(report_type::error, "Undefined variable name", expr_ident);
			return nullptr;
		}
		return sym->value;
	} else if (isa<ast::expr_member_access>(expr)) {
		auto expr_ma = (ast::expr_member_access *)expr.get();

		auto lhs_value = codegen(state, expr_ma->lhs);
		auto lhs_type = expr_resulting_type(state, expr_ma->lhs);
		auto lhs_object = (type_object *)lhs_type.get();
		auto lhs_custom = lhs_object->object_type;

		auto ident = &((ast::expr_ident *)expr_ma->rhs.get())->ident;

		int index = lhs_custom->index_of(ident->name);

		auto ptr =
			state.Builder.CreateStructGEP(lhs_custom->get_llvm_struct_type(state), lhs_value, index);

		return ptr;
	} else {
		state.report_message(report_type::error, "Not a valid lvalue", expr.get());
		return nullptr;
	}
}

} // namespace catalyst::compiler::codegen
