// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"

#include "../object_type.hpp"

namespace catalyst::compiler::codegen {

type_class::type_class(const std::string &name, std::vector<member> const &members)
	: type_custom("class", name) {
	this->members = members;
}

type_class::type_class(const std::string &name, std::shared_ptr<type_class> super, std::vector<member> const &members)
	: type_custom("class", name) {
	this->members = members;
	this->super = super;
}

std::shared_ptr<type_class> type_class::unknown() {
	static auto u = std::make_shared<type_class>("<unknown>", std::vector<member> {});
	return u;
}

std::shared_ptr<type> type::create_class(const std::string &name,
                                         std::vector<member> const &members) {
	return std::make_shared<type_class>(name, members);
}

std::shared_ptr<type> type::create_class(const std::string &name,
                                          std::shared_ptr<type_class> super,
                                          std::vector<member> const &members) {
	return std::make_shared<type_class>(name, super, members);
}

llvm::Type *type_class::get_llvm_struct_type(state &state) const {
	if (structType == nullptr) {
		std::vector<llvm::Type *> fields;
		if (super != nullptr) {
			fields.push_back(super->get_llvm_struct_type(state));
		}

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

		auto self = const_cast<type_class *>(this);
		self->structType = llvm::StructType::create(*state.TheContext, fields, name, false);
	}
	return structType;
}

llvm::Type *type_class::get_llvm_type(state &state) const {
	return llvm::PointerType::get(*state.TheContext, 0);
}

std::string type_class::get_fqn() const {
	std::string base = super ? std::string(":") + super->name : "";
	std::string fqn = "class(" + name + base + "){";
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

bool type_class::is_valid() {
	for (const auto &mmbr : members) {
		if (!mmbr.type->is_valid())
			return false;
	}
	if (super == type_class::unknown()) {
		return false;
	}
	return true;
}

void type_class::copy_from(type_class &other) {
	this->name = other.name;
	this->members.clear();
	this->members = other.members;
	this->super = other.super;
}

llvm::Value *type_class::get_sizeof(catalyst::compiler::codegen::state &state) {
	// Use trick from http://nondot.org/sabre/LLVMNotes/SizeOf-OffsetOf-VariableSizedStructs.txt
	// to mimic a sizeof()

	auto constant = llvm::Constant::getNullValue(get_llvm_type(state)->getPointerTo());
	auto size = state.Builder.CreateConstGEP1_64(get_llvm_struct_type(state), constant, 1, "sizep");
	return state.Builder.CreatePtrToInt(size, llvm::IntegerType::get(*state.TheContext, 64),
	                                    "sizei");
}

member_locator type_class::get_member(const std::string &name) {
	auto own_member = type_custom::get_member(name);
	if (!own_member.member && super) {
		return super->get_member(name);
	}
	return own_member;
}

int type_class::get_member_index_in_llvm_struct(member *member) {
	int offset = super ? 1 : 0;
	return offset + type_custom::get_member_index_in_llvm_struct(member);
}

} // namespace catalyst::compiler::codegen
