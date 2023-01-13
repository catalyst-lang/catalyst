// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <typeinfo>

#include "catalyst/rtti.hpp"
#include "expr.hpp"
#include "expr_type.hpp"
#include "value.hpp"
#include "llvm/IR/Value.h"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
llvm::Value *codegen(codegen::state &state, ast::expr_ptr expr,
                     std::shared_ptr<type> expecting_type) {
	if (isa<ast::expr_ident>(expr)) {
		return codegen(state, *(ast::expr_ident *)expr.get(), expecting_type);
	} else if (isa<ast::expr_literal_numeric>(expr)) {
		return codegen(state, *(ast::expr_literal_numeric *)expr.get(), expecting_type);
	} else if (isa<ast::expr_literal_bool>(expr)) {
		return codegen(state, *(ast::expr_literal_bool *)expr.get(), expecting_type);
	} else if (isa<ast::expr_binary_arithmetic>(expr)) {
		return codegen(state, *(ast::expr_binary_arithmetic *)expr.get(), expecting_type);
	} else if (isa<ast::expr_unary_arithmetic>(expr)) {
		return codegen(state, *(ast::expr_unary_arithmetic *)expr.get(), expecting_type);
	} else if (isa<ast::expr_binary_logical>(expr)) {
		return codegen(state, *(ast::expr_binary_logical *)expr.get(), expecting_type);
	} else if (isa<ast::expr_assignment>(expr)) {
		return codegen(state, *(ast::expr_assignment *)expr.get(), expecting_type);
	} else if (isa<ast::expr_call>(expr)) {
		return codegen(state, *(ast::expr_call *)expr.get(), expecting_type);
	} else if (isa<ast::expr_member_access>(expr)) {
		return codegen(state, *(ast::expr_member_access *)expr.get(), expecting_type);
	} else if (isa<ast::expr_cast>(expr)) {
		return codegen(state, *(ast::expr_cast *)expr.get(), expecting_type);
	}

	state.report_message(report_type::error, "Expression type unsupported", expr.get());

	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_numeric &expr,
                     std::shared_ptr<type> expecting_type) {
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
	auto *semantics = &llvm::APFloat::IEEEhalf();
	if (definitive_type->get_fqn() == "f16")
		semantics = &llvm::APFloat::IEEEhalf();
	else if (definitive_type->get_fqn() == "f32")
		semantics = &llvm::APFloat::IEEEsingle();
	else if (definitive_type->get_fqn() == "f64")
		semantics = &llvm::APFloat::IEEEdouble();
	else if (definitive_type->get_fqn() == "f128")
		semantics = &llvm::APFloat::IEEEquad();
	else if (definitive_type->get_fqn() == "f80")
		semantics = &llvm::APFloat::x87DoubleExtended();
	f = llvm::APFloat(*semantics, fstr.str());

	if (expr.sign < 0)
		f.changeSign();
	if (expr.exponent.has_value()) {
		auto apexp = llvm::APFloat(pow(10, (double)expr.exponent.value()));
		bool losesInfo;
		apexp.convert(*semantics, llvm::RoundingMode::TowardZero, &losesInfo);
		f.multiply(apexp, llvm::RoundingMode::TowardZero);
	}

	return llvm::ConstantFP::get(*state.TheContext, f);
}

llvm::Value *codegen(codegen::state &state, ast::expr_literal_bool &expr,
                     std::shared_ptr<type> expecting_type) {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(1, expr.value));
}

llvm::Value *codegen_this(codegen::state &state, ast::expr_ident &expr,
                     std::shared_ptr<type> expecting_type) {
	auto fn_type = (type_function*)state.current_function_symbol->type.get();
	if (!fn_type) {
		state.report_message(report_type::error, "`this` referenced outside of function.", &expr);
		return nullptr;
	}
	if (!fn_type->is_method()) {
		state.report_message(report_type::error, "`this` referenced in non-member function.", &expr);
		state.report_message(report_type::info, "In function:", state.current_function_symbol->ast_node);
		return nullptr;
	}
	return state.current_function->getArg(0);
}

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr,
                     std::shared_ptr<type> expecting_type) {
	if (expr.ident.name == "this") return codegen_this(state, expr, expecting_type);

	// Look this variable up in the function.
	auto *symbol = state.scopes.find_named_symbol(expr.ident.name);
	if (!symbol) {
		state.report_message(report_type::error, "Unknown variable name", &expr);
		return nullptr;
	}

	if (llvm::isa<llvm::Function>(symbol->value)) {
		auto *a = (llvm::Function *)symbol->value;
		// return state.Builder.CreateLoad(a->getType(), a, expr.ident.name.c_str());
		auto *container = state.Builder.CreateAlloca(llvm::PointerType::get(*state.TheContext, 0));
		state.Builder.CreateStore(a, container);
		return state.Builder.CreateLoad(container->getAllocatedType(), container,
		                                expr.ident.name.c_str());
	} else if (llvm::isa<llvm::GlobalVariable>(symbol->value)) {
		auto *a = (llvm::GlobalVariable *)symbol->value;
		return state.Builder.CreateLoad(a->getValueType(), a, expr.ident.name.c_str());
	} else if (llvm::isa<llvm::Argument>(symbol->value)) {
		auto *a = (llvm::Argument *)symbol->value;
		return a;
	} else {
		auto *a = (llvm::AllocaInst *)symbol->value;
		if (llvm::isa<llvm::StructType>(a->getAllocatedType())) {
			// if (expecting_type == nullptr) {
			//  return the pointer
			return a;
			//} else if (type id(*expecting_type) == type id(type_object)) {
			//	auto *a_struct = (type_struct*)symbol->type.get();
			//	return state.Builder.CreateLoad(a_struct->get_llvm_type(state), a);
			//} else {
			//	state.report_message(report_type::error, "Unknown cast expected", expr);
			//	return nullptr;
			//}
		} else {
			return state.Builder.CreateLoad(a->getAllocatedType(), a, expr.ident.name.c_str());
		}
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_arithmetic &expr,
                     std::shared_ptr<type> expecting_type) {
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
		state.report_message(report_type::error, "Operator not implemented", &expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_unary_arithmetic &expr,
                     std::shared_ptr<type> expecting_type) {
	auto rhs = codegen(state, expr.rhs);

	if (rhs == nullptr)
		return nullptr;

	switch (expr.op) {
	case ast::expr_unary_arithmetic::op_t::complement:
		return state.Builder.CreateXor(rhs, -1, "xortmp");
	case ast::expr_unary_arithmetic::op_t::negate:
		return state.Builder.CreateNeg(rhs, "negtmp");
	default:
		state.report_message(report_type::error, "Operator not implemented", &expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_binary_logical &expr,
                     std::shared_ptr<type> expecting_type) {
	state.report_message(report_type::error, "expr_binary_logical: Not implemented", &expr);
	return nullptr;
}

llvm::Value *structure_to_pointer(codegen::state &state, llvm::Value *struct_val) {
	llvm::IRBuilder<> TmpB(&state.current_function->getEntryBlock(), state.current_function->getEntryBlock().begin());
	auto ptr = TmpB.CreateAlloca(struct_val->getType(), nullptr, "struct_ptr");
	state.Builder.CreateStore(struct_val, ptr);
	return ptr;
}

llvm::Value *codegen_call_new(codegen::state &state, symbol *sym, ast::expr_call &expr, llvm::Value *this_) {
	if (!isa<type_custom>(sym->type)) {
		// should never happen, but just to be sure we report an error if somebody calls this function in the
		// future with a non type_custom type.
		state.report_message(report_type::error, "Constructor call performed on non-constructor function", &expr);
		return nullptr;
	}
	auto sym_c = (type_custom*)sym->type.get();

	llvm::Value *new_object = nullptr;

	if (isa<type_struct>(sym->type)) {
		new_object = state.Builder.CreateAlloca(sym_c->get_llvm_type(state), nullptr, "new");
	} else {
		state.report_message(report_type::error, "TODO " CATALYST_AT, &expr);
		return nullptr;
	}

	state.Builder.CreateCall(sym_c->init_function, { new_object });

	auto key = sym_c->name + "." + "new";
	if (state.symbol_table.contains(key)) {
		auto sym_new = &state.symbol_table[key];
		auto new_call = codegen_call(state, sym_new, expr, new_object);
	} else if (!expr.parameters.empty()) {
		state.report_message(report_type::error, std::string("No constructor has been defined on type `") + sym_c->name + "`", &expr);
		return nullptr;
	}

	return new_object;
}

llvm::Value *codegen_call(codegen::state &state, symbol *sym, ast::expr_call &expr, llvm::Value *this_) {
	llvm::Function *CalleeF;
	if (isa<type_function>(sym->type)) {
		CalleeF = (llvm::Function *)sym->value;
		if (!CalleeF) {
			state.report_message(report_type::error, "Unknown function referenced (symbol undefined)", &expr);
			return nullptr;
		}
	} else if (isa<type_custom>(sym->type)) {
		// constructor
		return codegen_call_new(state, sym, expr, this_);
	} else {
		state.report_message(report_type::error, "Unknown function referenced", &expr);
		return nullptr;
	}

	auto type = (type_function*)sym->type.get();

	// If argument mismatch error.
	if (type->parameters.size() != expr.parameters.size()) {
		// TODO, make ast_node from parameters for reporting errors
		std::stringstream str;
		str << "expected " << type->parameters.size() << ", but got " << expr.parameters.size();

		state.report_message(report_type::error, "Incorrect number of arguments passed", &expr,
								str.str());
		return nullptr;
	}

	// Generate arguments
	std::vector<llvm::Value *> ArgsV;
	if (this_) {
		// method call, prepend the this pointer
		ArgsV.push_back(this_);
	}
	for (unsigned i = 0, e = expr.parameters.size(); i != e; ++i) {
		auto arg_type = expr_resulting_type(state, expr.parameters[i], type->parameters[i]);
		auto arg = codegen(state, expr.parameters[i]);
		if (!arg_type->equals(type->parameters[i])) {
			// TODO: warn if casting happens
			arg = arg_type->cast_llvm_value(state, arg, type->parameters[i].get());
		}

		if (llvm::isa<llvm::StructType>(arg->getType())) {
			// we want a structure pointer instead
			arg = structure_to_pointer(state, arg);
		}

		ArgsV.push_back(arg);
		if (!ArgsV.back())
			return nullptr;
	}

	llvm::CallInst *callinstr = nullptr;
	if (llvm::isa<llvm::Function>(sym->value)) {
		// This is a straight function value
		if (isa<type_void>(type->return_type)) {
			callinstr = state.Builder.CreateCall(CalleeF, ArgsV);
		} else {
			callinstr = state.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
		}
	} else if (llvm::isa<llvm::AllocaInst>(sym->value)) {
		// This is a function pointer
		llvm::Value *ptr = state.Builder.CreateLoad(sym->value->getType(), sym->value);
		auto *fty = (llvm::FunctionType *)sym->type->get_llvm_type(state);
		if (isa<type_void>(type->return_type)) {
			callinstr = state.Builder.CreateCall(fty, ptr, ArgsV);
		} else {
			callinstr = state.Builder.CreateCall(fty, ptr, ArgsV, "ptrcalltmp");
		}
	} else {
		state.report_message(report_type::error, "unsupported base type for function call",	&expr);
		return nullptr;
	}

	// Revisit the arguments to add attributes. This can only be done after the llvm::CallInst is
	// generated.
	int method_offset = this_ ? 1 : 0;
	for (unsigned i = 0, e = expr.parameters.size(); i != e; ++i) {
		auto arg_type = expr_resulting_type(state, expr.parameters[i], type->parameters[i]);
		if (isa<type_object>(arg_type)) {
			auto to = (type_object *)arg_type.get();
			if (isa<type_struct>(to->object_type)) {
				callinstr->addParamAttr(i + method_offset, llvm::Attribute::NoUndef);
				callinstr->addParamAttr(i + method_offset, llvm::Attribute::getWithByValType(*state.TheContext, to->object_type->get_llvm_type(state)));
			}
		}
	}
	return callinstr;
}

llvm::Value *codegen(codegen::state &state, ast::expr_call &expr, std::shared_ptr<type> expecting_type) {
	if (isa<ast::expr_ident>(expr.lhs)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		// Look up the name in the global module table.
		auto sym = state.scopes.find_named_symbol(callee.ident.name);
		return codegen_call(state, sym, expr, nullptr);
	} else {
		state.report_message(report_type::error, "Virtual functions not implemented", &expr);
		return nullptr;
	}
}

llvm::Value *codegen(codegen::state &state, ast::expr_member_access &expr,
                     std::shared_ptr<type> expecting_type) {
	auto lhs_value = codegen(state, expr.lhs);
	auto lhs_type = expr_resulting_type(state, expr.lhs);

	if (!isa<type_object>(lhs_type)) {
		state.report_message(report_type::error, "Member access can only be performed on an object",
		                     expr.lhs.get());
		return nullptr;
	}

	auto lhs_object = (type_object *)lhs_type.get();
	auto lhs_struct = lhs_object->object_type;

	if (isa<ast::expr_ident>(expr.rhs)) {
		auto ident = &((ast::expr_ident *)expr.rhs.get())->ident;

		if (ident->name == "this") {
			state.report_message(report_type::error, "`this` is a reserved identifier", expr.rhs.get());
			state.report_message(report_type::help, "`this` can't be used to the right side of `.`. It can only be the left-most in a chain of member accesses.");
			return nullptr;
		}

		int index = lhs_struct->index_of(ident->name);

		if (llvm::isa<llvm::StructType>(lhs_value->getType())) {
			// we expect a pointer, but we got a structure value
			lhs_value = structure_to_pointer(state, lhs_value);
		}

		auto ptr = state.Builder.CreateStructGEP(lhs_struct->get_llvm_type(state), lhs_value, index);

		auto rhs_type = lhs_struct->members[index].type;
		if (isa<type_object>(rhs_type) && (!expecting_type || !isa<type_object>(expecting_type))) {
			// member is a struct or class, return the pointer if we don't request the
			// object value itself
			return ptr;
		}

		return state.Builder.CreateLoad(rhs_type->get_llvm_type(state), ptr);
	} else if (isa<ast::expr_call>(expr.rhs)) {
		auto call = (ast::expr_call *)expr.rhs.get();

		if (!isa<ast::expr_ident>(call->lhs)) {
			state.report_message(report_type::error, "Expected identifier", expr.lhs.get());
			return nullptr;
		}

		auto &ident = ((ast::expr_ident *)call->lhs.get())->ident;
		auto sym = state.scopes.find_named_symbol(lhs_struct->name + "." + ident.name);
		auto this_ = lhs_value;

		return codegen_call(state, sym, *call, this_);
	} else {
		state.report_message(report_type::error, "Identifier or name expected", expr.rhs.get());
		return nullptr;
	}
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
			                     "probably failing assignment due to type mismatch", rhs.get());
			return;
		}
	}

	if (isa<type_object>(rhs_type)) {
		auto to = (type_object *)rhs_type.get();
		if (isa<type_struct>(to->object_type)) {	
			auto size = to->object_type->get_sizeof(state);
			if (llvm::isa<llvm::PointerType>(rhs_value->getType())) {
				state.Builder.CreateMemCpy(dest_ptr, llvm::MaybeAlign(0), rhs_value, 
					llvm::MaybeAlign(0), size);
			} else if (llvm::isa<llvm::StructType>(rhs_value->getType())) {
				state.Builder.CreateStore(rhs_value, dest_ptr);
			} else {
				state.report_message(report_type::error, "Unexpected storage class", rhs.get());
			}
			return;
		}
	}

	state.Builder.CreateStore(rhs_value, dest_ptr);
}

llvm::Value *codegen(codegen::state &state, ast::expr_assignment &expr,
                     std::shared_ptr<type> expecting_type) {
	auto lvalue = get_lvalue(state, expr.lhs);
	if (lvalue == nullptr) {
		state.report_message(report_type::error, "assignment must be towards an lvalue", &expr);
		return nullptr;
	}

	codegen_assignment(state, lvalue, expr_resulting_type(state, expr.lhs), expr.rhs);

	return lvalue;
}

llvm::Value *codegen(codegen::state &state, ast::expr_cast &expr,
                     std::shared_ptr<type> expecting_type) {
	auto expr_type = expr_resulting_type(state, expr.lhs);
	auto value = codegen(state, expr.lhs);
	return expr_type->cast_llvm_value(state, value, type::create(state, expr.type).get());
}

} // namespace catalyst::compiler::codegen
