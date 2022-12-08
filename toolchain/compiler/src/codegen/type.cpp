// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "type.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<type> type::create(const std::string &name) {
	if (name == "i32")
		return std::make_shared<type_i32>();
	if (name == "i64")
		return std::make_shared<type_i64>();
	if (name == "i32")
		return std::make_shared<type_i32>();
	if (name == "i64")
		return std::make_shared<type_i64>();

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
	return (this->fqn == other.fqn);
}

bool type::operator!=(const type &other) const {
	return (this->fqn != other.fqn);
}

std::string type::get_fqn() const {
	return fqn;
}

llvm::Type *type_undefined::get_llvm_type(state &state) const {
	return llvm::Type::getVoidTy(*state.TheContext);
}

llvm::Type *type_i32::get_llvm_type(state &state) const {
	return llvm::Type::getInt32Ty(*state.TheContext);
}

llvm::Type *type_i64::get_llvm_type(state &state) const {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Type *type_u32::get_llvm_type(state &state) const {
	return llvm::Type::getInt32Ty(*state.TheContext);
}

llvm::Type *type_u64::get_llvm_type(state &state) const {
	return llvm::Type::getInt64Ty(*state.TheContext);
}

llvm::Type *type_float::get_llvm_type(state &state) const {
	return llvm::Type::getFloatTy(*state.TheContext);
}

llvm::Type *type_double::get_llvm_type(state &state) const {
	return llvm::Type::getDoubleTy(*state.TheContext);
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