// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iostream>
#include <sstream>
#include <typeinfo>
#include <cmath>

#include "expr.hpp"
#include "expr_type.hpp"
#include "value.hpp"
#include "llvm/IR/Value.h"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
llvm::Value *codegen(codegen::state &state, ast::expr_ptr expr, std::shared_ptr<type> expecting_type) {
	if (typeid(*expr) == typeid(ast::expr_ident)) {
		return codegen(state, *(ast::expr_ident *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_literal_numeric)) {
		return codegen(state, *(ast::expr_literal_numeric *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_literal_bool)) {
		return codegen(state, *(ast::expr_literal_bool *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_binary_arithmetic)) {
		return codegen(state, *(ast::expr_binary_arithmetic *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_unary_arithmetic)) {
		return codegen(state, *(ast::expr_unary_arithmetic *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_binary_logical)) {
		return codegen(state, *(ast::expr_binary_logical *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_assignment)) {
		return codegen(state, *(ast::expr_assignment *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_call)) {
		return codegen(state, *(ast::expr_call *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_member_access)) {
		return codegen(state, *(ast::expr_member_access *)expr.get(), expecting_type);
	} else if (typeid(*expr) == typeid(ast::expr_cast)) {
		return codegen(state, *(ast::expr_cast *)expr.get(), expecting_type);
	}

	state.report_message(report_type::error, "Expression type unsupported", *expr);

	return {nullptr};
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr, std::shared_ptr<type> expecting_type) {
	auto expr_type = expr_resulting_type(state, expr);
	std::shared_ptr<type> definitive_type;
	if (expecting_type != nullptr && expecting_type->is_assignable_from(expr_type)) {
		definitive_type = expecting_type;
	} else {
		definitive_type = expr_type;
	}
	if (definitive_type->get_fqn() == "u128")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(128, expr.integer, false));
	if (definitive_type->get_fqn() == "u64")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, expr.integer, false));
	if (definitive_type->get_fqn() == "u32")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(32, expr.integer, false));
	if (definitive_type->get_fqn() == "u16")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(16, expr.integer, false));
	if (definitive_type->get_fqn() == "u8")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(8, expr.integer, false));

	if (definitive_type->get_fqn() == "i128")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(128, expr.integer, true));
	if (definitive_type->get_fqn() == "i64")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, expr.integer, true));
	if (definitive_type->get_fqn() == "i32")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(32, expr.integer, true));
	if (definitive_type->get_fqn() == "i16")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(16, expr.integer, true));
	if (definitive_type->get_fqn() == "i8")
		return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(8, expr.integer, true));

	// Floating Point
	std::stringstream fstr;
	fstr << expr.integer;
	if (expr.fraction.has_value()) {
		fstr << ".";
		fstr << expr.fraction.value();
	}
	llvm::APFloat f = llvm::APFloat(0.0);
	auto* semantics = &llvm::APFloat::IEEEhalf();
	if (definitive_type->get_fqn() == "f16") semantics = &llvm::APFloat::IEEEhalf();
	else if (definitive_type->get_fqn() == "f32") semantics = &llvm::APFloat::IEEEsingle();
	else if (definitive_type->get_fqn() == "f64") semantics = &llvm::APFloat::IEEEdouble();
	else if (definitive_type->get_fqn() == "f128") semantics = &llvm::APFloat::IEEEquad();
	else if (definitive_type->get_fqn() == "f80") semantics = &llvm::APFloat::x87DoubleExtended();
	f = llvm::APFloat(*semantics, fstr.str());

	if (expr.sign < 0) f.changeSign();
	if (expr.exponent.has_value()) {
		auto apexp = llvm::APFloat(pow(10, (double)expr.exponent.value()));
		bool losesInfo;
		apexp.convert(*semantics, llvm::RoundingMode::TowardZero, &losesInfo);
		f.multiply(apexp, llvm::RoundingMode::TowardZero);
	}

	return llvm::ConstantFP::get(*state.TheContext, f);

}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr, std::shared_ptr<type> expecting_type) {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(1, expr.value));
}

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr, std::shared_ptr<type> expecting_type) {
	// Look this variable up in the function.
	auto *symbol = state.scopes.find_named_symbol(expr.ident.name);
	if (!symbol)
		state.report_message(report_type::error, "Unknown variable name", expr);
		

	if (llvm::isa<llvm::Function>(symbol->value)) {
		auto *a = (llvm::Function *)symbol->value;
		//return state.Builder.CreateLoad(a->getType(), a, expr.ident.name.c_str());
		auto *container = state.Builder.CreateAlloca(llvm::PointerType::get(*state.TheContext, 0));
		state.Builder.CreateStore(a, container);
		return state.Builder.CreateLoad(container->getAllocatedType(), container, expr.ident.name.c_str());
	} if (llvm::isa<llvm::GlobalVariable>(symbol->value)) {
		auto *a = (llvm::GlobalVariable *)symbol->value;
		return state.Builder.CreateLoad(a->getValueType(), a, expr.ident.name.c_str());
	} else {
		auto *a = (llvm::AllocaInst *)symbol->value;
		return state.Builder.CreateLoad(a->getAllocatedType(), a, expr.ident.name.c_str());
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_arithmetic &expr, std::shared_ptr<type> expecting_type) {
	auto lhs = codegen(state, expr.lhs);
	auto rhs = codegen(state, expr.rhs);

	if (lhs == nullptr || rhs == nullptr)
		return nullptr;

	// below only works for primitive types

	auto expr_type = expr_resulting_type(state, expr);
	auto lhs_type = expr_resulting_type(state, expr.lhs);
	auto rhs_type = expr_resulting_type(state, expr.rhs);

	if (*lhs_type != *expr_type) {
		lhs = lhs_type->cast_llvm_value(state, lhs, expr_type.get());
	}

	if (*rhs_type != *expr_type) {
		rhs = rhs_type->cast_llvm_value(state, rhs, expr_type.get());
	}

	switch (expr.op) {
	case ast::expr_binary_arithmetic::op_t::plus: {
		auto add = lhs_type->create_add(state, lhs, rhs_type, rhs);
		lhs_type->cast_llvm_value(state, add, expr_type.get());
		return add;
	}
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

llvm::Value *codegen(codegen::state &state, ast::expr_unary_arithmetic &expr, std::shared_ptr<type> expecting_type) {
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

llvm::Value *codegen(codegen::state &state, ast::expr_binary_logical &expr, std::shared_ptr<type> expecting_type) {
	state.report_message(report_type::error, "expr_binary_logical: Not implemented", expr);
	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_call &expr, std::shared_ptr<type> expecting_type) {
	if (typeid(*expr.lhs) == typeid(ast::expr_ident)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		// Look up the name in the global module table.
		// llvm::Function *CalleeF = state.TheModule->getFunction(callee.name);
		auto sym = state.scopes.find_named_symbol(callee.ident.name);
		auto type = (type_function *)sym->type.get();
		auto CalleeF = (llvm::Function *)sym->value;
		if (!CalleeF) {
			state.report_message(report_type::error, "Unknown function referenced", callee);
			return nullptr;
		}

		// If argument mismatch error.
		if (type->parameters.size() != expr.parameters.size()) {
			// TODO, make ast_node from parameters for reporting errors
			std::stringstream str;
			str << "expected " << type->parameters.size() << ", but got " << expr.parameters.size();

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
				arg = arg_type->cast_llvm_value(state, arg, type->parameters[i].get());
			}
			ArgsV.push_back(arg);
			if (!ArgsV.back())
				return nullptr;
		}

		if (llvm::isa<llvm::Function>(sym->value)) {
			// This is a straight function value
			return state.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
		} else if (llvm::isa<llvm::AllocaInst>(sym->value)) {
			// This is a function pointer
			llvm::Value* ptr = state.Builder.CreateLoad(sym->value->getType(), sym->value);
			llvm::FunctionType* fty = (llvm::FunctionType*)sym->type->get_llvm_type(state);
			return state.Builder.CreateCall(fty, ptr, ArgsV, "ptrcalltmp");
		} else {
			state.report_message(report_type::error, "unsupported base type for function call", expr);
			return nullptr;
		}
	} else {
		state.report_message(report_type::error, "Virtual functions not implemented", expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_member_access &expr, std::shared_ptr<type> expecting_type) {
	state.report_message(report_type::error, "expr_member_access: Not implemented", expr);
	return nullptr;
}

void codegen_assignment(codegen::state &state, llvm::Value *dest_ptr,
                        std::shared_ptr<type> dest_type, ast::expr_ptr rhs) {
	auto rhs_value = codegen(state, rhs, dest_type);
	auto rhs_type = expr_resulting_type(state, rhs, dest_type);

	if (*dest_type != *rhs_type) {
		// need to cast
		auto new_rhs_value = rhs_type->cast_llvm_value(state, rhs_value, dest_type.get());
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

llvm::Value *codegen(codegen::state &state, ast::expr_assignment &expr, std::shared_ptr<type> expecting_type) {
	auto lvalue = get_lvalue(state, expr.lhs);
	if (lvalue == nullptr) {
		state.report_message(report_type::error, "assignment must be towards an lvalue", expr);
		return nullptr;
	}

	codegen_assignment(state, lvalue, expr_resulting_type(state, expr.lhs), expr.rhs);

	return lvalue;
}

llvm::Value *codegen(codegen::state &state, ast::expr_cast &expr, std::shared_ptr<type> expecting_type) {
	auto expr_type = expr_resulting_type(state, expr.lhs);
	auto value = codegen(state, expr.lhs);
	return expr_type->cast_llvm_value(state, value, type::create(state, expr.type).get());
}

} // namespace catalyst::compiler::codegen
