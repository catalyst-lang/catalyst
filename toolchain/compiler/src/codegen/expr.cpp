// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "expr.hpp"

namespace catalyst::compiler::codegen {

llvm::Value *codegen(codegen::state &state, ast::expr &expr) {
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal &expr);
llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr);
llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr);

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr) {
	// Look this variable up in the function.
	llvm::Value *V = state.NamedValues[expr.name];
	//if (!V)
		//LogErrorV("Unknown variable name");
	return V;
}

} // namespace catalyst::compiler::codegen