// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <format>
#include <typeinfo>

#include "expr.hpp"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
llvm::Value *codegen(codegen::state &state, ast::expr_ptr expr) {
	if (typeid(*expr) == typeid(ast::expr_ident)) {
		return codegen(state, *(ast::expr_ident *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_literal_numeric)) {
		return codegen(state, *(ast::expr_literal_numeric *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_literal_bool)) {
		return codegen(state, *(ast::expr_literal_bool *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_binary_arithmetic)) {
		return codegen(state, *(ast::expr_binary_arithmetic *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_unary_arithmetic)) {
		return codegen(state, *(ast::expr_unary_arithmetic *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_binary_logical)) {
		return codegen(state, *(ast::expr_binary_logical *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_call)) {
		return codegen(state, *(ast::expr_call *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_member_access)) {
		return codegen(state, *(ast::expr_member_access *)expr.get());
	}

	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr) {
	return llvm::ConstantInt::get(state.TheContext, llvm::APInt(64, expr.integer, true));
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr) {
	return llvm::ConstantInt::get(state.TheContext, llvm::APInt(1, expr.value));
}

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr) {
	// Look this variable up in the function.
	llvm::Value *V = state.scopes.get_named_value(expr.name);
	if (!V)
		state.report_error("Unknown variable name", expr);
	return V;
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_arithmetic &expr) {
	auto lhs = codegen(state, expr.lhs);
	auto rhs = codegen(state, expr.rhs);

	if (lhs == nullptr || rhs == nullptr)
		return nullptr;

	switch (expr.op) {
	case ast::expr_binary_arithmetic::op_t::plus:
		return state.Builder.CreateAdd(lhs, rhs, "addtmp");
	case ast::expr_binary_arithmetic::op_t::minus:
		return state.Builder.CreateSub(lhs, rhs, "subtmp");
	case ast::expr_binary_arithmetic::op_t::times:
		return state.Builder.CreateMul(lhs, rhs, "multmp");
	case ast::expr_binary_arithmetic::op_t::div:
		return state.Builder.CreateSDiv(lhs, rhs, "divtmp");
	default:
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_unary_arithmetic &expr) {
	state.report_error("Not implemented");
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_logical &expr) {
	state.report_error("Not implemented");
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_call &expr) {
	if (typeid(*expr.lhs) == typeid(ast::expr_ident)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		// Look up the name in the global module table.
		llvm::Function *CalleeF = state.TheModule->getFunction(callee.name);
		if (!CalleeF) {
			state.report_error("Unknown function referenced", callee);
			return nullptr;
		}

		// If argument mismatch error.
		if (CalleeF->arg_size() != expr.parameters.size()) {
			// todo, make positional from parameters
			state.report_error("Incorrect number of arguments passed", callee,
			                   std::format("expected {}, but got {}", CalleeF->arg_size(),
			                               expr.parameters.size()));
			return nullptr;
		}

		std::vector<llvm::Value *> ArgsV;
		for (unsigned i = 0, e = expr.parameters.size(); i != e; ++i) {
			ArgsV.push_back(codegen(state, expr.parameters[i]));
			if (!ArgsV.back())
				return nullptr;
		}

		return state.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
	} else {
		state.report_error("Virtual functions not implemented");
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_member_access &expr) {
	state.report_error("Not implemented");
	return nullptr;
}

} // namespace catalyst::compiler::codegen