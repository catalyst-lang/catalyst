// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "object_type.hpp"

#include "catalyst/rtti.hpp"

#include <algorithm>

namespace catalyst::compiler::codegen {

member_locator type_custom::get_member(const std::string &name) {
	for (int i = 0; i < members.size(); i++) {
		if (members[i].name == name) {
			return member_locator(&members[i], this);
		}
	}
	return member_locator::invalid();
}

member_locator type_custom::get_member(const type_function *function) {
	for (int i = 0; i < members.size(); i++) {
		if (members[i].type.get() == function) {
			return member_locator(&members[i], this);
		}
	}
	return member_locator::invalid();
}

int type_custom::get_member_index_in_llvm_struct(member *member) {
	// the llvm_index is the index passed to a GEP instruction. So functions should be left out.
	// Note that function types should still be counted in if they are a function pointer.
	int llvm_index = 0;
	for (auto &m : members) {
		if (&m == member) {
			return llvm_index;
		}
		if (!isa<ast::decl_fn>(m.decl)) {
			llvm_index++;
		}
	}
	return -1;
}

int type_custom::get_member_index_in_llvm_struct(const member_locator &member_locator) {
	if (member_locator.residence != this)
		return -1;
	return get_member_index_in_llvm_struct(member_locator.member);
}

type_object::type_object(std::shared_ptr<type_custom> object_type)
	: type("object"), object_type(object_type) {}

llvm::Type *type_object::get_llvm_type(state &state) const {
	return object_type->get_llvm_type(state);
}

std::string type_object::get_fqn() const { return object_type->name; }

bool type_object::is_valid() const { return object_type->is_valid(); }

llvm::Value *type_object::cast_llvm_value(codegen::state &state, llvm::Value *value,
                                          const type &to) const {
	if (to.is_assignable_from(this->object_type)) {
		return value;
	} else {
		return nullptr;
	}
}

bool type_object::is_assignable_from(const std::shared_ptr<type> &type) const {
	if (isa<type_virtual>(object_type)) {
		auto assigning_to_class = std::static_pointer_cast<type_virtual>(object_type);
		if (isa<type_virtual>(type)) {
			auto assigning_from_class = std::static_pointer_cast<type_virtual>(type);
			return assigning_to_class->is_assignable_from(assigning_from_class);
		}
	}

	return false;
}

type_virtual::type_virtual(const std::string &fqn, const std::string &name,
                           std::vector<member> const &members)
	: type_custom(fqn, name) {
	this->members = members;
	this->name = name;
}

type_virtual::type_virtual(const std::string &fqn, const std::string &name,
                           const std::vector<std::shared_ptr<type_virtual>> &super,
                           std::vector<member> const &members)
	: type_custom(fqn, name) {
	this->name = name;
	this->members = members;
	this->super = super;
}

bool type_virtual::is_assignable_from(const std::shared_ptr<type> &type) const {
	if (type.get() == this)
		return true;

	if (isa<type_virtual>(type)) {
		auto const &from = std::static_pointer_cast<type_virtual>(type);
		return std::any_of(
			from->super.cbegin(), from->super.cend(),
			[&](const std::shared_ptr<type_virtual> &s) { return this->is_assignable_from(s); });
	}

	return false;
}

} // namespace catalyst::compiler::codegen
