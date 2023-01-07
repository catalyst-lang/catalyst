// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "type.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<type> type::create_builtin(const std::string &name) {
	if (name == "i8")
		return std::make_shared<type_i8>();
	if (name == "i16")
		return std::make_shared<type_i16>();
	if (name == "i32")
		return std::make_shared<type_i32>();
	if (name == "i64")
		return std::make_shared<type_i64>();
	if (name == "i128")
		return std::make_shared<type_i128>();
	if (name == "isize")
		return std::make_shared<type_isize>();
	if (name == "u8")
		return std::make_shared<type_u8>();
	if (name == "u16")
		return std::make_shared<type_u16>();
	if (name == "u32")
		return std::make_shared<type_u32>();
	if (name == "u64")
		return std::make_shared<type_u64>();
	if (name == "u128")
		return std::make_shared<type_u128>();
	if (name == "usize")
		return std::make_shared<type_usize>();
	if (name == "f16")
		return std::make_shared<type_f16>();
	if (name == "f32")
		return std::make_shared<type_f32>();
	if (name == "f64")
		return std::make_shared<type_f64>();
	if (name == "f128")
		return std::make_shared<type_f128>();
	if (name == "f80")
		return std::make_shared<type_f80>();
	if (name == "bool")
		return std::make_shared<type_bool>();

	if (name.empty())
		return std::make_shared<type_undefined>();

	return std::make_shared<type_undefined>();
}

std::shared_ptr<type> type::create_builtin(const ast::type &ast_type) {
	return create_builtin(ast_type.ident.name);
}

std::shared_ptr<type> type::create(codegen::state &state, const std::string &name) {
	auto sym = state.scopes.find_named_symbol(name);
	if (sym != nullptr) {
		if (typeid(*sym->type) == typeid(type_struct)) {
			return std::make_shared<type_object>(std::dynamic_pointer_cast<type_struct>(sym->type));
		}
	}

	return create_builtin(name);
}

std::shared_ptr<type> type::create(codegen::state &state, const ast::type &ast_type) {
	return create(state, ast_type.ident.name);
}

std::shared_ptr<type> type::create_function(const std::shared_ptr<type> &return_type) {
	return std::make_shared<type_function>(return_type);
}

std::shared_ptr<type> type::create_function(const std::shared_ptr<type> &return_type,
                                            const std::vector<std::shared_ptr<type>> &parameters) {
	return std::make_shared<type_function>(return_type, parameters);
}

bool type::operator==(const type &other) const { return (this->get_fqn() == other.get_fqn()); }

bool type::operator!=(const type &other) const { return (this->get_fqn() != other.get_fqn()); }

std::string type::get_fqn() const { return fqn; }

llvm::Value *type::cast_llvm_value(state &state, llvm::Value *value, type *to) { return nullptr; }

bool type::is_assignable_from(const std::shared_ptr<type> &type) const { return false; }
llvm::Value *type::create_add(state &state, llvm::Value *lhs, std::shared_ptr<type> rhs_type,
                              llvm::Value *rhs) {
	return nullptr;
}

llvm::Type *type_undefined::get_llvm_type(state &state) {
	return llvm::Type::getVoidTy(*state.TheContext);
}

llvm::Type *type_bool::get_llvm_type(state &state) {
	return llvm::Type::getInt1Ty(*state.TheContext);
}

llvm::Constant *type_bool::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(1, 0));
}

llvm::Type *type_i8::get_llvm_type(state &state) {
	return llvm::Type::getInt8Ty(*state.TheContext);
}

llvm::Constant *type_i8::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(8, 0));
}

llvm::Type *type_i16::get_llvm_type(state &state) {
	return llvm::Type::getInt16Ty(*state.TheContext);
}

llvm::Constant *type_i16::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(16, 0));
}

llvm::Type *type_i32::get_llvm_type(state &state) {
	return llvm::Type::getInt32Ty(*state.TheContext);
}

llvm::Constant *type_i32::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(32, 0));
}

llvm::Type *type_i64::get_llvm_type(state &state) {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Constant *type_i64::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, 0));
}

llvm::Type *type_i128::get_llvm_type(state &state) {
	return llvm::Type::getInt128Ty(*state.TheContext);
}

llvm::Constant *type_i128::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(128, 0));
}

llvm::Type *type_isize::get_llvm_type(state &state) {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Constant *type_isize::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, 0));
}

llvm::Type *type_u8::get_llvm_type(state &state) {
	return llvm::Type::getInt8Ty(*state.TheContext);
}

llvm::Constant *type_u8::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(8, 0));
}

llvm::Type *type_u16::get_llvm_type(state &state) {
	return llvm::Type::getInt16Ty(*state.TheContext);
}

llvm::Constant *type_u16::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(16, 0));
}

llvm::Type *type_u32::get_llvm_type(state &state) {
	return llvm::Type::getInt32Ty(*state.TheContext);
}

llvm::Constant *type_u32::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(32, 0));
}

llvm::Type *type_u64::get_llvm_type(state &state) {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Constant *type_u64::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, 0));
}

llvm::Type *type_u128::get_llvm_type(state &state) {
	return llvm::Type::getInt128Ty(*state.TheContext);
}

llvm::Constant *type_u128::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(128, 0));
}

llvm::Type *type_usize::get_llvm_type(state &state) {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Constant *type_usize::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantInt::get(*state.TheContext, llvm::APInt(64, 0));
}

llvm::Type *type_f16::get_llvm_type(state &state) {
	return llvm::Type::getHalfTy(*state.TheContext);
}

llvm::Constant *type_f16::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantFP::get(llvm::Type::getHalfTy(*state.TheContext), 0.0);
}

llvm::Type *type_f32::get_llvm_type(state &state) {
	return llvm::Type::getFloatTy(*state.TheContext);
}

llvm::Constant *type_f32::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantFP::get(llvm::Type::getFloatTy(*state.TheContext), 0.0);
}

llvm::Type *type_f64::get_llvm_type(state &state) {
	return llvm::Type::getDoubleTy(*state.TheContext);
}

llvm::Constant *type_f64::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*state.TheContext), 0.0);
}

llvm::Type *type_f128::get_llvm_type(state &state) {
	return llvm::Type::getFP128Ty(*state.TheContext);
}

llvm::Constant *type_f128::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantFP::get(llvm::Type::getFP128Ty(*state.TheContext), 0.0);
}

llvm::Type *type_f80::get_llvm_type(state &state) {
	return llvm::Type::getX86_FP80Ty(*state.TheContext);
}

llvm::Constant *type_f80::get_llvm_constant_zero(codegen::state &state) const {
	return llvm::ConstantFP::get(llvm::Type::getX86_FP80Ty(*state.TheContext), 0.0);
}

llvm::Value *type_primitive::cast_llvm_value(state &state, llvm::Value *value, type *to) {
	auto p_to = dynamic_cast<type_primitive *>(to);
	if (p_to == nullptr) {
		// state.report_message("Converting between types that aren't both primitives");
		// assert(false);
		return nullptr;
	}
	if (!is_float && !p_to->is_float) {
		if (is_signed) {
			return state.Builder.CreateSExtOrTrunc(value, p_to->get_llvm_type(state));
		} else {
			return state.Builder.CreateZExtOrTrunc(value, p_to->get_llvm_type(state));
		}
	} else if (!is_float && p_to->is_float) {
		if (is_signed) {
			return state.Builder.CreateSIToFP(value, p_to->get_llvm_type(state));
		} else {
			return state.Builder.CreateUIToFP(value, p_to->get_llvm_type(state));
		}
	} else if (is_float && !p_to->is_float) {
		if (p_to->is_signed) {
			return state.Builder.CreateFPToSI(value, p_to->get_llvm_type(state));
		} else {
			return state.Builder.CreateFPToUI(value, p_to->get_llvm_type(state));
		}
	} else if (is_float && p_to->is_float) {
		if (p_to->bits > bits) {
			// Sign extend
			return state.Builder.CreateFPExt(value, p_to->get_llvm_type(state));
		} else {
			return state.Builder.CreateFPTrunc(value, p_to->get_llvm_type(state));
		}
	}

	assert(false);
	return nullptr;
}

llvm::Value *type_primitive::create_add(codegen::state &state, llvm::Value *value,
                                        std::shared_ptr<type> rhs_type, llvm::Value *rhs) {
	auto p_rhs = dynamic_cast<type_primitive *>(rhs_type.get());
	if (p_rhs == nullptr) {
		return nullptr;
	}

	if (!is_float && !p_rhs->is_float) {
		return state.Builder.CreateAdd(value, rhs, "add");
	} else {
		// at least one of us is a fp
		if (!is_float)
			value = cast_llvm_value(state, value, rhs_type.get());
		if (!p_rhs->is_float)
			rhs = rhs_type->cast_llvm_value(state, rhs, this);
		return state.Builder.CreateFAdd(value, rhs, "add");
	}
}

bool type_primitive::is_assignable_from(const std::shared_ptr<type> &type) const {
	auto p_type = dynamic_cast<type_primitive *>(type.get());
	if (p_type != nullptr) {
		return true;
	} else {
		return type::is_assignable_from(type);
	}
}

llvm::Type *type_function::get_llvm_type(state &state) {
	std::vector<llvm::Type *> params;
	for (const auto &param : parameters) {
		params.push_back(param->get_llvm_type(state));
	}

	return llvm::FunctionType::get(return_type->get_llvm_type(state), params, false);
}

std::string type_function::get_fqn() const {
	std::string fqn = "fn(";
	bool first = true;
	for (auto param : parameters) {
		if (!first) {
			fqn += ",";
		} else {
			first = false;
		}
		fqn += param->get_fqn();
	}
	fqn += ")->";
	fqn += return_type->get_fqn();
	return fqn;
}

bool type_function::is_valid() {
	if (!return_type->is_valid()) return false;
	for (auto param : parameters) {
		if (!param->is_valid()) return false;
	}
	return true;
}

type_struct::type_struct(const std::string &name, std::map<std::string, std::shared_ptr<type>> const &members)
	: type_custom("struct", name) {
	this->members.insert(members.begin(), members.end());
}

std::shared_ptr<type> type::create_struct(const std::string &name, std::map<std::string, std::shared_ptr<type>> const &members) {
	return std::make_shared<type_struct>(name, members);
}

llvm::Type *type_struct::get_llvm_type(state &state) {
	if (structType == nullptr) {
		std::vector<llvm::Type *> fields;
		for (const auto &member : members) {
			fields.push_back(member.second->get_llvm_type(state));
		}

		structType = llvm::StructType::create(*state.TheContext, fields, name, true);
	}
	return structType;
}

std::string type_struct::get_fqn() const {
	std::string fqn = "struct(" + name + "){";
	bool first = true;
	for (const auto &[name, type] : members) {
		if (!first) {
			fqn += ",";
		} else {
			first = false;
		}
		fqn += name;
		fqn += ":";
		fqn += type->get_fqn();
	}
	fqn += "}";
	return fqn;
}

bool type_struct::is_valid() {
	for (const auto &[name, type] : members) {
		if (!type->is_valid()) return false;
	}
	return true;
}

void type_struct::copy_from(type_struct &other) {
	this->name = other.name;
	this->members.clear();
	this->members.insert(other.members.begin(), other.members.end());
}


type_object::type_object(std::shared_ptr<type_custom> object_type)
	: type("object"), object_type(object_type) {
}

llvm::Type *type_object::get_llvm_type(state &state) {
	return object_type->get_llvm_type(state);
}

std::string type_object::get_fqn() const {
	return object_type->name;
}

bool type_object::is_valid() {
	return object_type->is_valid();
}

} // namespace catalyst::compiler::codegen
