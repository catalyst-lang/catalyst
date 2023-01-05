// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "value.hpp"

namespace catalyst::compiler::codegen {

// get an lvalue
// see https://en.cppreference.com/w/cpp/language/value_category
// in the Catalyst compiler, an lvalue is an llvm::Value address to something.
llvm::Value * get_lvalue(codegen::state &state, ast::expr_ptr expr) {
    if (typeid(*expr) == typeid(ast::expr_ident)) {
		auto expr_ident = (ast::expr_ident*)expr.get();
		auto sym = state.scopes.find_named_symbol(expr_ident->ident.name);
		if (sym == nullptr) {
			state.report_message(report_type::error, "Undefined variable name", *expr_ident);
			return nullptr;
		}
	    return sym->value;
	}
    return nullptr;
}

} // namespace catalyst::compiler::codegen
