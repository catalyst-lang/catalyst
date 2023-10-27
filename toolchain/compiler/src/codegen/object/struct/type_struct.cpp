// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"

#include "../object_type.hpp"

namespace catalyst::compiler::codegen {

type_struct::type_struct(const std::string &name, std::vector<member> const &members)
	: type_custom("struct", name) {
	this->members = members;
}

std::shared_ptr<type> type::create_struct(const std::string &name, std::vector<member> const &members) {
	return std::make_shared<type_struct>(name, members);
}

llvm::StructType *type_struct::get_llvm_struct_type(state &state) const {
	if (structType == nullptr) {
		std::vector<llvm::Type *> fields;
		for (const auto &member : members) {
			if (!isa<ast::decl_fn>(member.decl)) {
				if (isa<type_function>(member.type)) {
					// var that points to function
					fields.push_back(llvm::PointerType::get(*state.TheContext, 0));
				} else {
					fields.push_back(member.type->get_llvm_type(state));
				}
			}
		}

		auto self = const_cast<type_struct*>(this);
		self->structType = llvm::StructType::create(*state.TheContext, fields, name, true);
	}
	return structType;
}

llvm::Type *type_struct::get_llvm_type(state &state) const {
	return get_llvm_struct_type(state);
}

std::string type_struct::get_fqn() const {
	std::string fqn = "struct(" + name + "){";
	bool first = true;
	for (const auto &mmbr : members) {
		if (!first) {
			fqn += ",";
		} else {
			first = false;
		}
		fqn += mmbr.name;
		fqn += ":";
		fqn += mmbr.type->get_fqn();
	}
	fqn += "}";
	return fqn;
}

bool type_struct::is_valid() const {
	for (const auto &mmbr : members) {
		if (!mmbr.type->is_valid()) return false;
	}
	return true;
}

void type_struct::copy_from(type_struct &other) {
	this->name = other.name;
	this->members.clear();
	this->members = other.members;
}

llvm::Value* type_struct::get_sizeof(catalyst::compiler::codegen::state &state) {
	// Use trick from http://nondot.org/sabre/LLVMNotes/SizeOf-OffsetOf-VariableSizedStructs.txt
	// to mimic a sizeof()
	
	auto constant = llvm::Constant::getNullValue(get_llvm_type(state)->getPointerTo());
	auto size = state.Builder.CreateConstGEP1_64(get_llvm_struct_type(state), constant, 1, "sizep");
	return state.Builder.CreatePtrToInt(size, llvm::IntegerType::get(*state.TheContext, 64), "sizei");
}

} // namespace catalyst::compiler::codegen
