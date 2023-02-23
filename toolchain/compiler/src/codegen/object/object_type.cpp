// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"

#include "object_type.hpp"

namespace catalyst::compiler::codegen {

member_locator type_custom::get_member(const std::string &name) {
	for (int i = 0; i < members.size(); i++) {
		if (members[i].name == name) {
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

bool type_object::is_valid() { return object_type->is_valid(); }

llvm::Value *type_object::cast_llvm_value(codegen::state &state, llvm::Value *value,
                                          const type &to) const {
	if (to.is_assignable_from(this->object_type)) {
		return value;
	} else {
		return nullptr;
	}
}

bool type_object::is_assignable_from(const std::shared_ptr<type> &type) const {
	if (isa<type_class>(object_type)) {
		auto base_class = std::static_pointer_cast<type_class>(object_type);
		if (isa<type_class>(type)) {
			auto descending_class = std::static_pointer_cast<type_class>(type);
			while (descending_class.get() != nullptr && descending_class.get() != base_class.get())
				descending_class = descending_class->super;
			return descending_class.get() == base_class.get();
		}
	}

	return false;
}

} // namespace catalyst::compiler::codegen
