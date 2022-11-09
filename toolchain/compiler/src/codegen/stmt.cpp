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
llvm::Value *codegen(codegen::state &state, ast::statement_ptr stmt) {
	if (typeid(*stmt) == typeid(ast::statement_expr)) {
		return codegen(state, *(ast::statement_expr *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_var)) {
		return codegen(state, *(ast::statement_var *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_return)) {
		return codegen(state, *(ast::statement_return *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_const)) {
		return codegen(state, *(ast::statement_const *)stmt.get());
	} else {
		state.report_error("unsupported statement type");
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
