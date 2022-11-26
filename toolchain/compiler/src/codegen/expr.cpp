// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <sstream>
#include <typeinfo>

#include "expr.hpp"
#include "value.hpp"

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
	} else if (typeid(*expr) == typeid(ast::expr_assignment)) {
		return codegen(state, *(ast::expr_assignment *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_call)) {
		return codegen(state, *(ast::expr_call *)expr.get());
	} else if (typeid(*expr) == typeid(ast::expr_member_access)) {
		return codegen(state, *(ast::expr_member_access *)expr.get());
	}

	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr) {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, expr.integer, true));
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr) {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(1, expr.value));
}

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr) {
	// Look this variable up in the function.
	auto *symbol = state.scopes.find_named_symbol(expr.name);
	if (!symbol)
		state.report_error("Unknown variable name", expr);
	llvm::AllocaInst *a = (llvm::AllocaInst *)symbol->value;
	return state.Builder.CreateLoad(a->getAllocatedType(), a, expr.name.c_str());
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
		state.report_error("Operator not implemented");
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_unary_arithmetic &expr) {
	auto rhs = codegen(state, expr.rhs);

	if (rhs == nullptr)
		return nullptr;

	switch (expr.op) {
	case ast::expr_unary_arithmetic::op_t::complement:
		return state.Builder.CreateXor(rhs, -1, "xortmp");
	case ast::expr_unary_arithmetic::op_t::negate:
		return state.Builder.CreateNeg(rhs, "negtmp");
	default:
		state.report_error("Operator not implemented");
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_logical &expr) {
	state.report_error("expr_binary_logical: Not implemented");
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
			// TODO, make positional from parameters
			std::stringstream str;
			str << "expected " << CalleeF->arg_size() << ", but got " << expr.parameters.size();

			state.report_error("Incorrect number of arguments passed", callee, str.str());
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
	state.report_error("expr_member_access: Not implemented");
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_assignment &expr) {
	auto lvalue = get_lvalue(state, expr.lhs);
	if (lvalue == nullptr) {
		state.report_error("assignment must be towards an lvalue");
		return nullptr;
	}
	
	state.Builder.CreateStore(codegen(state, expr.rhs), lvalue);
	return lvalue;
}

} // namespace catalyst::compiler::codegen
