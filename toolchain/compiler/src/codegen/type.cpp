// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "type.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<type> type::create(const std::string &name) {
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
	if (name == "fp16")
		return std::make_shared<type_fp16>();
	if (name == "fp32")
		return std::make_shared<type_fp32>();
	if (name == "fp64")
		return std::make_shared<type_fp64>();
	if (name == "fp128")
		return std::make_shared<type_fp128>();
	if (name == "fp80")
		return std::make_shared<type_fp80>();
	if (name == "bool")
		return std::make_shared<type_bool>();

	if (name == "")
		return std::make_shared<type_undefined>();

	return std::make_shared<type_undefined>();
}

std::shared_ptr<type> type::create_function(const std::shared_ptr<type>& return_type) {
	return std::make_shared<type_function>(return_type);
}

std::shared_ptr<type> type::create_function(const std::shared_ptr<type>& return_type,
                                            const std::vector<std::shared_ptr<type>> &parameters) {
	return std::make_shared<type_function>(return_type, parameters);
}

std::shared_ptr<type> type::create(const ast::type &ast_type) {
	return create(ast_type.ident.name);
}

bool type::operator==(const type &other) const {
	return (this->get_fqn() == other.get_fqn());
}

bool type::operator!=(const type &other) const {
	return (this->get_fqn() != other.get_fqn());
}

std::string type::get_fqn() const {
	return fqn;
}

llvm::Type *type_undefined::get_llvm_type(state &state) const {
	return llvm::Type::getVoidTy(*state.TheContext);
}

llvm::Type *type_bool::get_llvm_type(state &state) const {
	return llvm::Type::getInt1Ty(*state.TheContext);
}

llvm::Type *type_i8::get_llvm_type(state &state) const {
	return llvm::Type::getInt8Ty(*state.TheContext);
}

llvm::Type *type_i16::get_llvm_type(state &state) const {
	return llvm::Type::getInt16Ty(*state.TheContext);
}

llvm::Type *type_i32::get_llvm_type(state &state) const {
	return llvm::Type::getInt32Ty(*state.TheContext);
}

llvm::Type *type_i64::get_llvm_type(state &state) const {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Type *type_i128::get_llvm_type(state &state) const {
	return llvm::Type::getInt128Ty(*state.TheContext);
}

llvm::Type *type_isize::get_llvm_type(state &state) const {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Type *type_u8::get_llvm_type(state &state) const {
	return llvm::Type::getInt8Ty(*state.TheContext);
}

llvm::Type *type_u16::get_llvm_type(state &state) const {
	return llvm::Type::getInt16Ty(*state.TheContext);
}

llvm::Type *type_u32::get_llvm_type(state &state) const {
	return llvm::Type::getInt32Ty(*state.TheContext);
}

llvm::Type *type_u64::get_llvm_type(state &state) const {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Type *type_u128::get_llvm_type(state &state) const {
	return llvm::Type::getInt128Ty(*state.TheContext);
}

llvm::Type *type_usize::get_llvm_type(state &state) const {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Type *type_fp16::get_llvm_type(state &state) const {
	return llvm::Type::getHalfTy(*state.TheContext);
}

llvm::Type *type_fp32::get_llvm_type(state &state) const {
	return llvm::Type::getFloatTy(*state.TheContext);
}

llvm::Type *type_fp64::get_llvm_type(state &state) const {
	return llvm::Type::getDoubleTy(*state.TheContext);
}

llvm::Type *type_fp128::get_llvm_type(state &state) const {
	return llvm::Type::getFP128Ty(*state.TheContext);
}

llvm::Type *type_fp80::get_llvm_type(state &state) const {
	return llvm::Type::getX86_FP80Ty(*state.TheContext);
}

llvm::Type *type_function::get_llvm_type(state &state) const {
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


} // namespace catalyst::compiler::codegen