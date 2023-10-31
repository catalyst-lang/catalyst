// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <typeinfo>

#include "catalyst/rtti.hpp"
#include "../runtime.hpp"
#include "expr.hpp"
#include "expr_type.hpp"
#include "function_overloading.hpp"
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
	auto f = llvm::APFloat(0.0);
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
                     [[maybe_unused]] std::shared_ptr<type> expecting_type) {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(1, expr.value));
}

llvm::Value *codegen_this(codegen::state &state, ast::expr_ident &expr,
                          std::shared_ptr<type> expecting_type) {
	auto fn_type = (type_function *)state.current_function_symbol->type.get();
	if (!fn_type) {
		state.report_message(report_type::error, "`this` referenced outside of function.", &expr);
		return nullptr;
	}
	if (!fn_type->is_method()) {
		state.report_message(report_type::error, "`this` referenced in non-member function.",
		                     &expr);
		state.report_message(report_type::info,
		                     "In function:", state.current_function_symbol->ast_node);
		return nullptr;
	}
	return state.current_function->getArg(0);
}

llvm::Value *codegen(codegen::state &state, ast::expr_ident &expr,
                     std::shared_ptr<type> expecting_type) {
	if (expr.ident.name == "this")
		return codegen_this(state, expr, expecting_type);

	// Look this variable up in the function.
	symbol *symbol;
	if (isa<type_function>(expecting_type)) {
		symbol = find_function_overload(
			state, expr.ident.name, std::static_pointer_cast<type_function>(expecting_type), &expr);
	} else {
		symbol = state.scopes.find_named_symbol(expr.ident.name);
	}

	if (!symbol) {
		state.report_message(report_type::error, "Unknown variable name", &expr);
		return nullptr;
	}

	if (isa<type_void>(symbol->type)) {
		return nullptr;
	} else if (isa<type_ns>(symbol->type)) {
		state.report_message(report_type::error, "Namespaces can't be referenced directly", &expr);
		return nullptr;
	} else if (llvm::isa<llvm::Function>(symbol->value)) {
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

llvm::Value *structure_to_pointer(codegen::state &state, llvm::Value *struct_val) {
	llvm::IRBuilder<> TmpB(&state.current_function->getEntryBlock(),
	                       state.current_function->getEntryBlock().begin());
	auto ptr = TmpB.CreateAlloca(struct_val->getType(), nullptr, "struct_ptr");
	state.Builder.CreateStore(struct_val, ptr);
	return ptr;
}

llvm::Value *get_sizeof_ptr(codegen::state &state) {
	auto ptr_type = llvm::PointerType::get(*state.TheContext, 0);
	auto size = state.Builder.CreateConstGEP1_64(ptr_type, llvm::Constant::getNullValue(ptr_type),
	                                             1, "sizep");
	return state.Builder.CreatePtrToInt(size, llvm::IntegerType::get(*state.TheContext, 64),
	                                    "sizei");
}

llvm::Value *codegen_call_new(codegen::state &state, std::shared_ptr<type> stype,
                              ast::expr_call &expr, llvm::Value *this_) {
	if (!stype) {
		return nullptr;
	}

	if (!isa<type_custom>(stype)) {
		// should never happen, but just to be sure we report an error if somebody calls this
		// function in the future with a non type_custom type.
		state.report_message(report_type::error,
		                     "Constructor call performed on non-constructor function", &expr);
		return nullptr;
	}
	auto sym_c = (type_custom *)stype.get();

	llvm::Value *new_object = nullptr;

	if (isa<type_struct>(stype)) {
		new_object = state.Builder.CreateAlloca(sym_c->get_llvm_type(state), nullptr, "new");
	} else if (isa<type_class>(stype)) {
		auto class_size = stype->get_sizeof(state);
		new_object = state.Builder.CreateCall(state.target->get_malloc(), {class_size}, "instance");
	} else {
		state.report_message(report_type::error, "TODO " CATALYST_AT, &expr);
		return nullptr;
	}

	state.Builder.CreateCall(sym_c->init_function, {new_object});

	auto key = sym_c->name + "." + "new";
	auto sym_new = find_function_overload(state, key, expr, type::create_builtin("void"));
	if (sym_new) {
		auto new_call =
			codegen_call(state, sym_new, expr, new_object, type::create_builtin("void"));
	} else if (!expr.parameters.empty()) {
		state.report_message(
			report_type::error,
			std::string("No constructor has been defined on type `") + sym_c->name + "`", &expr);
		return nullptr;
	}

	return new_object;
}

llvm::Value *codegen_call(codegen::state &state, std::shared_ptr<codegen::type> stype,
                          llvm::Value *value, ast::expr_call &expr, llvm::Value *this_,
                          std::shared_ptr<type> expecting_type) {
	if (!stype)
		return nullptr;
	llvm::Function *CalleeF;
	if (isa<type_function>(stype)) {
		CalleeF = (llvm::Function *)value;
		if (!CalleeF) {
			state.report_message(report_type::error,
			                     "Unknown function referenced (symbol undefined)", &expr);
			return nullptr;
		}
	} else if (isa<type_custom>(stype)) {
		// constructor
		return codegen_call_new(state, stype, expr, this_);
	} else {
		state.report_message(report_type::error, "Unknown function referenced", &expr);
		return nullptr;
	}

	auto type = (type_function *)stype.get();

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
	for (size_t i = 0, e = expr.parameters.size(); i != e; ++i) {
		auto arg_type = expr_resulting_type(state, expr.parameters[i], type->parameters[i]);
		auto arg = codegen(state, expr.parameters[i], type->parameters[i]);
		if (!arg)
			return nullptr;
		if (!arg_type->equals(type->parameters[i])) {
			// TODO: warn if casting happens
			arg = arg_type->cast_llvm_value(state, arg, *type->parameters[i]);
			if (arg == nullptr) {
				state.report_message(report_type::error,
				                     std::string("Cannot cast parameter type from `") +
				                         arg_type->get_fqn() + "` to expected type `" +
				                         type->parameters[i]->get_fqn() + "`",
				                     expr.parameters[i].get());
				return nullptr;
			}
		}

		if (llvm::isa<llvm::StructType>(arg->getType())) {
			// we want a structure pointer instead
			arg = structure_to_pointer(state, arg);
		}

		ArgsV.push_back(arg);
		if (!ArgsV.back())
			return nullptr;
	}

	// Virtual functions
	if (type->is_virtual()) {
		if (!this_) {
			state.report_message(report_type::error,
			                     "calling a virtual function without context of `this`", &expr);
			return nullptr;
		}

		auto c = std::dynamic_pointer_cast<type_class>(type->method_of);
		auto member = c->get_member(type);
		auto metadata_type = c->get_llvm_metadata_struct_type(state);
		auto metadata_location =
			state.Builder.CreateStructGEP(c->get_llvm_struct_type(state), this_, 0);
		auto metadata_object = state.Builder.CreateLoad(
			llvm::PointerType::get(*state.TheContext, 0), metadata_location);
		auto vtable = state.Builder.CreateConstGEP1_32(metadata_type, metadata_object, 0, "vtable");
		int index = c->get_virtual_member_index(state, member);
		value = state.Builder.CreateConstGEP2_32(metadata_type->elements()[0], vtable, 0, index,
		                                         "vfnptr");
	}

	llvm::CallInst *callinstr = nullptr;
	if (llvm::isa<llvm::Function>(value)) {
		// This is a straight function value
		if (isa<type_void>(type->return_type)) {
			callinstr = state.Builder.CreateCall((llvm::Function *)value, ArgsV);
		} else {
			callinstr = state.Builder.CreateCall((llvm::Function *)value, ArgsV, "calltmp");
		}
	} else if (llvm::isa<llvm::UnaryInstruction>(value) ||
	           llvm::isa<llvm::GetElementPtrInst>(value) || llvm::isa<llvm::Constant>(value)) {
		// This is a function pointer
		llvm::Value *ptr = state.Builder.CreateLoad(value->getType(), value);
		auto *fty = (llvm::FunctionType *)stype->get_llvm_type(state);
		if (isa<type_void>(type->return_type)) {
			callinstr = state.Builder.CreateCall(fty, ptr, ArgsV);
		} else {
			callinstr = state.Builder.CreateCall(fty, ptr, ArgsV, "ptrcalltmp");
		}
	} else {
		state.report_message(report_type::error, "unsupported base type for function call", &expr);
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
				callinstr->addParamAttr(
					i + method_offset,
					llvm::Attribute::getWithByValType(*state.TheContext,
				                                      to->object_type->get_llvm_type(state)));
			} else if (isa<type_class>(to->object_type)) {
				callinstr->addParamAttr(i + method_offset, llvm::Attribute::NoUndef);
			}
		}
	}
	return callinstr;
}

llvm::Value *codegen_call(codegen::state &state, symbol *sym, ast::expr_call &expr,
                          llvm::Value *this_, std::shared_ptr<type> expecting_type) {
	if (!sym)
		return nullptr;
	return codegen_call(state, sym->type, sym->value, expr, this_, expecting_type);
}

llvm::Value *codegen(codegen::state &state, ast::expr_call &expr,
                     std::shared_ptr<type> expecting_type) {
	if (isa<ast::expr_ident>(expr.lhs)) {
		auto &callee = *(ast::expr_ident *)expr.lhs.get();
		// Look up the name in the global module table.
		auto sym = find_function_overload(state, callee.ident.name, expr, expecting_type);
		return codegen_call(state, sym, expr, nullptr, expecting_type);
	} else {
		state.report_message(report_type::error, "Virtual functions not implemented", &expr);
		return nullptr;
	}
}

llvm::Value *get_super_typed_value(codegen::state &state, llvm::Value *this_,
                                   type_custom *this_type, type_custom *super_type) {
	if (isa<type_virtual>(this_type)) {
		if (this_type != super_type) {
			auto this_virtual = (type_virtual *)this_type;
			int index = this_virtual->get_super_index_in_llvm_struct(super_type);
			if (index >= 0) {
				return state.Builder.CreateStructGEP(this_virtual->get_llvm_struct_type(state),
				                                     this_, index, super_type->name + "_ptr");
			} else {
				for (auto const &s : this_virtual->super) {
					if (super_type->is_assignable_from(s)) {
						this_ = state.Builder.CreateStructGEP(
							this_virtual->get_llvm_struct_type(state), this_,
							this_virtual->get_super_index_in_llvm_struct(s.get()),
							s->name + "_ptr");
						return get_super_typed_value(state, this_, s.get(), super_type);
					}
				}
			}
		}
	}
	return this_;
}

llvm::Value *codegen(codegen::state &state, ast::expr_member_access &expr,
                     std::shared_ptr<type> expecting_type) {
	auto lhs_type = expr_resulting_type(state, expr.lhs);

	if (isa<type_ns>(lhs_type)) {
		auto current_scope = state.current_scope().get_fully_qualified_scope_name();
		state.scopes.enter_ns(std::static_pointer_cast<type_ns>(lhs_type));
		auto rhs_value = codegen(state, expr.rhs, expecting_type);
		state.scopes.leave();
		state.scopes.enter_fqn(current_scope);
		return rhs_value;
	}

	if (!isa<type_object>(lhs_type)) {
		state.report_message(report_type::error, "Member access can only be performed on an object",
		                     expr.lhs.get());
		return nullptr;
	}

	auto lhs_value = codegen(state, expr.lhs);
	auto lhs_object = (type_object *)lhs_type.get();
	auto lhs_custom = lhs_object->object_type;

	if (isa<ast::expr_ident>(expr.rhs)) {
		auto ident = &((ast::expr_ident *)expr.rhs.get())->ident;

		if (ident->name == "this") {
			state.report_message(report_type::error, "`this` is a reserved identifier",
			                     expr.rhs.get());
			state.report_message(report_type::help,
			                     "`this` can't be used to the right side of `.`. It can only be "
			                     "the left-most in a chain of member accesses.");
			return nullptr;
		}

		if (llvm::isa<llvm::StructType>(lhs_value->getType())) {
			// we expect a pointer, but we got a structure value
			lhs_value = structure_to_pointer(state, lhs_value);
		}

		auto member_loc = lhs_custom->get_member(ident->name);

		// if this is a virtual (class-like), get the sub-type
		if (isa<type_virtual>(lhs_custom)) {
			lhs_value =
				get_super_typed_value(state, lhs_value, lhs_custom.get(), member_loc.residence);
		}

		auto ptr = state.Builder.CreateStructGEP(
			member_loc.residence->get_llvm_struct_type(state), lhs_value,
			member_loc.residence->get_member_index_in_llvm_struct(member_loc));

		auto rhs_type = member_loc.member->type;
		if (isa<type_object>(rhs_type) && (!expecting_type || !isa<type_object>(expecting_type))) {
			auto to = (type_object *)rhs_type.get();
			if (isa<type_struct>(to->object_type)) {
				// member is a struct, return the pointer if we don't request the object value
				// itself Note that this is only valid for a struct. With a class, the pointer IS
				// the value, so we want to retrieve that value and dereference it to a pointer to a
				// class structure.
				return ptr;
			}
		}

		return state.Builder.CreateLoad(rhs_type->get_llvm_type(state), ptr);
	} else if (isa<ast::expr_call>(expr.rhs)) {
		auto call = (ast::expr_call *)expr.rhs.get();

		if (!isa<ast::expr_ident>(call->lhs)) {
			state.report_message(report_type::error, "Expected identifier", expr.lhs.get());
			return nullptr;
		}

		auto const &ident = ((ast::expr_ident *)call->lhs.get())->ident;

		auto member_loc = lhs_custom->get_member(ident.name);
		if (isa<ast::decl_fn>(member_loc.member->decl)) {
			auto sym = find_function_overload(
				state, member_loc.residence->name + "." + member_loc.member->name, *call,
				expecting_type);
			auto this_ = lhs_value;
			auto sym_fn_type = std::static_pointer_cast<type_function>(sym->type);
			if (!sym_fn_type->is_virtual()) {
				// make sure this_ is pointing to the correct subtype if we have multiple
				if (isa<type_virtual>(lhs_custom)) {
					// if the function is not in this_ directly
					this_ = get_super_typed_value(state, this_, lhs_custom.get(),
					                              sym_fn_type->method_of.get());
				}
			}
			return codegen_call(state, sym, *call, this_, expecting_type);
		} else {
			// function pointer
			if (!isa<type_function>(member_loc.member->type)) {
				state.report_message(report_type::error,
				                     std::string("`") + ident.name + "` is not a callable",
				                     member_loc.member->decl.get());
				return nullptr;
			} else {
				auto ptr = state.Builder.CreateStructGEP(
					member_loc.residence->get_llvm_struct_type(state), lhs_value,
					member_loc.residence->get_member_index_in_llvm_struct(member_loc));

				// auto this_ = lhs_value;
				//  no this available on such call
				return codegen_call(state, member_loc.member->type, ptr, *call, nullptr,
				                    expecting_type);
			}
		}
	} else {
		state.report_message(report_type::error, "Identifier or name expected", expr.rhs.get());
		return nullptr;
	}
}

void codegen_assignment(codegen::state &state, llvm::Value *dest_ptr,
                        std::shared_ptr<type> dest_type, ast::expr_ptr rhs) {
	auto rhs_value = codegen(state, rhs, dest_type);
	auto rhs_type = expr_resulting_type(state, rhs, dest_type);

	if (rhs_value == nullptr) {
		return;
	}

	if (*dest_type != *rhs_type) {
		// need to cast
		auto new_rhs_value = rhs_type->cast_llvm_value(state, rhs_value, *dest_type);
		if (new_rhs_value) {
			rhs_value = new_rhs_value;
		} else {
			// TODO casting
			state.report_message(report_type::error,
			                     std::string("Cannot assign value of type `") +
			                         rhs_type->get_fqn() + "` where type `" + dest_type->get_fqn() +
			                         "` is expected",
			                     rhs.get());
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

	if (isa<type_void>(rhs_type)) {
		state.report_message(report_type::warning, "Assigning variable of type void has no effect",
		                     rhs.get());
		return;
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
	return expr_type->cast_llvm_value(state, value, *type::create(state, expr.type));
}

} // namespace catalyst::compiler::codegen
