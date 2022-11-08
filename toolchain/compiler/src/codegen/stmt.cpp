// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <typeinfo>

#include "decl.hpp"
#include "stmt.hpp"
#include "expr.hpp"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
llvm::Value *codegen(codegen::state &state, ast::statement &stmt) {
	if (std::holds_alternative<ast::statement_expr>(stmt)) {
		return codegen(state, std::get<ast::statement_expr>(stmt));
	} else if (std::holds_alternative<ast::statement_var>(stmt)) {
		return codegen(state, std::get<ast::statement_var>(stmt));
	} else if (std::holds_alternative<ast::statement_return>(stmt)) {
		return codegen(state, std::get<ast::statement_return>(stmt));
	} else if (std::holds_alternative<ast::statement_const>(stmt)) {
		return codegen(state, std::get<ast::statement_const>(stmt));
	} else {
		state.report_error("unsupported body type");
	}

	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::statement_expr &stmt) {
	return codegen(state, stmt.expr);
}

llvm::Value *codegen(codegen::state &state, ast::statement_return &stmt) {
	return state.Builder.CreateRet(codegen(state, stmt.expr));
}

llvm::Value *codegen(codegen::state &state, ast::statement_var &stmt) {
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::statement_const &stmt) {
	return nullptr;
}


} // namespace catalyst::compiler::codegen