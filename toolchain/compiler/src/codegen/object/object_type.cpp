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

int type_custom::get_member_index_in_llvm_struct(member *member) const {
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

int type_custom::get_member_index_in_llvm_struct(const member_locator &member_locator) const {
	if (member_locator.residence != this)
		return -1;
	return get_member_index_in_llvm_struct(member_locator.member);
}

llvm::Value *type_custom::cast_llvm_value(codegen::state &state, llvm::Value *value,
                                          const type &to) const {
	return value;
}

type_object::type_object(catalyst::compiler::codegen::state &state, const std::string& type_fqn)
	: type("object"), object_type(state, type_fqn) {}

type_object::type_object(catalyst::compiler::codegen::state &state, std::shared_ptr<type_custom> custom_type)
	: type("object"), object_type(state, custom_type) {}

type_object::type_object(object_type_reference<type_custom> virtual_type)
	: type("object"), object_type(virtual_type) {}

llvm::Type *type_object::get_llvm_type(state &state) const {
	return object_type->get_llvm_type(state);
}

std::string type_object::get_fqn() const { return object_type->name; }

bool type_object::is_valid() const { return object_type->is_valid(); }

llvm::Value *type_object::cast_llvm_value(codegen::state &state, llvm::Value *value,
                                          const type &to) const {
	if (to.is_assignable_from(this->object_type.get())) {
		if (isa<type_object>(to)) {
			return this->object_type->cast_llvm_value(state, value,
			                                          *((type_object *)&to)->object_type.get());
		} else {
			return this->object_type->cast_llvm_value(state, value, to);
		}
	} else {
		return nullptr;
	}
}

bool type_object::is_assignable_from(const std::shared_ptr<type> &type) const {
	if (isa<type_virtual>(object_type.get())) {
		auto assigning_to_class = std::static_pointer_cast<type_virtual>(object_type.get());
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
                           const std::vector<object_type_reference<type_virtual>> &super,
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
			[&](const object_type_reference<type_virtual> &s) { return this->is_assignable_from(s.get()); });
	}

	return false;
}

bool type_virtual::is_downcastable_from(const std::shared_ptr<type_virtual> &from) const {
	if (from.get() == this)
		return true;

	return std::any_of(
			this->super.cbegin(), this->super.cend(),
			[&](const object_type_reference<type_virtual> &s) { return s->is_downcastable_from(from); });
}

std::vector<member_locator> type_virtual::get_virtual_members() {
	// TODO: we can probably cache this the first time we call it

	std::vector<member_locator> fns;

	for (auto const &s : super) {
		auto new_fns = s->get_virtual_members();
		fns.insert(fns.end(), new_fns.begin(), new_fns.end());
	}

	for (auto &m : members) {
		if (m.is_virtual()) {
			// check if this virtual member is already if the list
			auto is_override = [&](const member_locator &m2) {
				if (m.name != m2.member->name)
					return false;
				if (*m.type != *m2.member->type)
					return false;
				return true;
			};
			auto result = std::find_if(fns.begin(), fns.end(), is_override);
			if (result != fns.end()) {
				// override found!
				(*result) = member_locator(&m, this);
			} else {
				// new virtual function
				fns.emplace_back(&m, this);
			}
		}
	}

	return fns;
}

std::vector<member_locator> type_virtual::get_virtual_members(const std::string &name) {
	auto vmems = get_virtual_members();
	std::string canonical_name = name.substr(0, name.find_first_of('`'));
	std::erase_if(vmems, [&canonical_name](const member_locator &ml) {
		std::string other_canonical_name = ml.member->name;
		return other_canonical_name.substr(0, other_canonical_name.find_first_of('`')) != canonical_name;
	});
	return vmems;
}

int type_virtual::get_virtual_member_index(codegen::state &state, const member_locator &member) {
	// TODO: multiple inheritance
	auto fns = get_virtual_members();
	auto it = find(fns.begin(), fns.end(), member);
	if (it != fns.end()) {
		return it - fns.begin();
	} else {
		return -1;
	}
}

member_locator type_virtual::get_member(const std::string &name) {
	auto own_member = type_custom::get_member(name);
	if (!own_member.member && !super.empty()) {
		for (auto const &s : super) {
			auto m = s->get_member(name);
			if (m.is_valid())
				return m;
		}
	}
	return own_member;
}

member_locator type_virtual::get_member(const type_function *function) {
	auto own_member = type_custom::get_member(function);
	if (!own_member.member && !super.empty()) {
		for (auto const &s : super) {
			auto m = s->get_member(function);
			if (m.is_valid())
				return m;
		}
	}
	return own_member;
}

int type_virtual::get_member_index_in_llvm_struct(member *member) const {
	return super.size() + type_custom::get_member_index_in_llvm_struct(member);
}

int type_virtual::get_super_index_in_llvm_struct(type_custom *p) const {
	auto result =
		std::find_if(super.begin(), super.end(),
	                 [&](const object_type_reference<type_virtual> &p2) { return p == p2.get().get(); });
	if (result != super.end()) {
		size_t index = std::distance(super.begin(), result);
		return index;
	} else {
		return -1;
	}
}

} // namespace catalyst::compiler::codegen
