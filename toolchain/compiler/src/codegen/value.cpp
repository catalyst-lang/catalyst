// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "value.hpp"
#include "catalyst/rtti.hpp"
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

		auto member_loc = lhs_custom->get_member(ident->name);

		if (!member_loc.is_valid()) {
			state.report_message(report_type::error,
			                     std::string("Type `") + lhs_custom->name +
			                         "` does not have a member named `" + ident->name + "`",
			                     ident);
			return nullptr;
		}

		lhs_value = get_super_typed_value(state, lhs_value, lhs_custom.get(), member_loc.residence);

		auto ptr = state.Builder.CreateStructGEP(
			member_loc.residence->get_llvm_struct_type(state), lhs_value,
			member_loc.residence->get_member_index_in_llvm_struct(member_loc));

		return ptr;
	} else {
		state.report_message(report_type::error, "Not a valid lvalue", expr.get());
		return nullptr;
	}
}

} // namespace catalyst::compiler::codegen
