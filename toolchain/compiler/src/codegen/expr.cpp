// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iostream>
#include <sstream>
#include <typeinfo>

#include "expr.hpp"
#include "expr_type.hpp"
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

	return {nullptr};
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr) {

	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, expr.integer, true));
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr) {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(1, expr.value));
}

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr) {
	// Look this variable up in the function.
	auto *symbol = state.scopes.find_named_symbol(expr.ident.name);
	if (!symbol)
		state.report_message(report_type::error, "Unknown variable name", expr);
	llvm::AllocaInst *a = (llvm::AllocaInst *)symbol->value;
	return state.Builder.CreateLoad(a->getAllocatedType(), a, expr.ident.name.c_str());
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_arithmetic &expr) {
	auto lhs = codegen(state, expr.lhs);
	auto rhs = codegen(state, expr.rhs);

	if (lhs == nullptr || rhs == nullptr)
		return nullptr;

	// below only works for primitive types

	auto expr_type = expr_resulting_type(state, expr);
	auto lhs_type = expr_resulting_type(state, expr.lhs);
	auto rhs_type = expr_resulting_type(state, expr.rhs);

	if (*lhs_type != *expr_type) {
		lhs = lhs_type->cast_llvm_value(state, lhs, expr_type);
	}

	if (*rhs_type != *expr_type) {
		rhs = rhs_type->cast_llvm_value(state, rhs, expr_type);
	}

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
		state.report_message(report_type::error, "Operator not implemented", expr);
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
		state.report_message(report_type::error, "Operator not implemented", expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_logical &expr) {
	state.report_message(report_type::error, "expr_binary_logical: Not implemented", expr);
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_call &expr) {
	if (typeid(*expr.lhs) == typeid(ast::expr_ident)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		// Look up the name in the global module table.
		// llvm::Function *CalleeF = state.TheModule->getFunction(callee.name);
		auto sym = state.scopes.find_named_symbol(callee.ident.name);
		auto type = (type_function *)sym->type.get();
		llvm::Function *CalleeF = (llvm::Function *)sym->value;
		if (!CalleeF) {
			state.report_message(report_type::error, "Unknown function referenced", callee);
			return nullptr;
		}

		// If argument mismatch error.
		if (CalleeF->arg_size() != expr.parameters.size()) {
			// TODO, make ast_node from parameters for reporting errors
			std::stringstream str;
			str << "expected " << CalleeF->arg_size() << ", but got " << expr.parameters.size();

			state.report_message(report_type::error, "Incorrect number of arguments passed", callee,
			                     str.str());
			return nullptr;
		}

		std::vector<llvm::Value *> ArgsV;
		for (unsigned i = 0, e = expr.parameters.size(); i != e; ++i) {
			auto arg_type = expr_resulting_type(state, expr.parameters[i], type->parameters[i]);
			auto arg = codegen(state, expr.parameters[i]);
			if (!arg_type->equals(type->parameters[i])) {
				// TODO: warn if casting happens
				arg = arg_type->cast_llvm_value(state, arg, type->parameters[i]);
			}
			ArgsV.push_back(arg);
			if (!ArgsV.back())
				return nullptr;
		}

		return state.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
	} else {
		state.report_message(report_type::error, "Virtual functions not implemented", expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_member_access &expr) {
	state.report_message(report_type::error, "expr_member_access: Not implemented", expr);
	return nullptr;
}

void codegen_assignment(codegen::state &state, llvm::Value *dest_ptr,
                        std::shared_ptr<type> dest_type, ast::expr_ptr rhs) {
	auto rhs_value = codegen(state, rhs);
	auto rhs_type = expr_resulting_type(state, rhs, dest_type);

	if (*dest_type != *rhs_type) {
		// need to cast
		auto new_rhs_value = rhs_type->cast_llvm_value(state, rhs_value, dest_type);
		if (new_rhs_value) {
			rhs_value = new_rhs_value;
		} else {
			// TODO casting
			state.report_message(report_type::warning,
			                     "probably failing assignment due to type mismatch", *rhs);
			return;
		}
	}

	state.Builder.CreateStore(rhs_value, dest_ptr);
}

llvm::Value *codegen(codegen::state &state, ast::expr_assignment &expr) {
	auto lvalue = get_lvalue(state, expr.lhs);
	if (lvalue == nullptr) {
		state.report_message(report_type::error, "assignment must be towards an lvalue", expr);
		return nullptr;
	}

	codegen_assignment(state, lvalue, expr_resulting_type(state, expr.lhs), expr.rhs);

	return lvalue;
}

} // namespace catalyst::compiler::codegen
