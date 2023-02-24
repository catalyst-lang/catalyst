// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"

#include "../object_type.hpp"

namespace catalyst::compiler::codegen {

type_class::type_class(const std::string &name, std::vector<member> const &members)
	: type_custom("class", name) {
	this->members = members;
}

type_class::type_class(const std::string &name, std::shared_ptr<type_class> super,
                       std::vector<member> const &members)
	: type_custom("class", name) {
	this->members = members;
	this->super = super;
}

std::shared_ptr<type_class> type_class::unknown() {
	static auto u = std::make_shared<type_class>("<unknown>", std::vector<member>{});
	return u;
}

std::shared_ptr<type> type::create_class(const std::string &name,
                                         std::vector<member> const &members) {
	return std::make_shared<type_class>(name, members);
}

std::shared_ptr<type> type::create_class(const std::string &name, std::shared_ptr<type_class> super,
                                         std::vector<member> const &members) {
	return std::make_shared<type_class>(name, super, members);
}

llvm::Type *type_class::get_llvm_struct_type(state &state) const {
	if (structType == nullptr) {
		std::vector<llvm::Type *> fields;
		
		
		if (super != nullptr) {
			fields.push_back(super->get_llvm_struct_type(state));
		} else {
			// metadata pointer (vtable)
			// Only push it on a bare class, because it get's inherited otherwise
			fields.push_back(llvm::PointerType::get(*state.TheContext, 0));
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

bool type_class::is_valid() const {
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

member_locator type_class::get_member(const type_function *function) {
	auto own_member = type_custom::get_member(function);
	if (!own_member.member && super) {
		return super->get_member(function);
	}
	return own_member;
}

int type_class::get_member_index_in_llvm_struct(member *member) {
	// offset of 1, either for the metadata in a bare class, or the parent class otherwise.
	return 1 + type_custom::get_member_index_in_llvm_struct(member);
}

std::vector<member_locator> type_class::get_virtual_members(codegen::state &state) {
	// TODO: we can probably cache this the first time we call it

	std::vector<member_locator> fns;

	if (super != nullptr) {
		fns = super->get_virtual_members(state);
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
				if (!m.classifiers.contains(ast::decl_classifier::override_)) {
					// We can hide the error below as it get detected in check_decl_classifiers() with
					// better error messages 

					// state.report_message(report_type::error,
					//                      std::string("Function `") + name + "." + m.name +
					//                          "` is overriding a virtual function, but is missing "
					//                          "the 'override' keyword",
					//                      m.decl.get());
				}
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

int type_class::get_virtual_member_index(codegen::state &state, const member_locator& member) {
	auto fns = get_virtual_members(state);
	auto it = find(fns.begin(), fns.end(), member);
	if (it != fns.end()) 
    {
        return it - fns.begin();
    }
    else {
        return -1;
    }
}

llvm::StructType *type_class::get_llvm_metadata_struct_type(codegen::state &state) {
	if (metadata_struct_type == nullptr) {
		auto vmems = get_virtual_members(state);
		std::vector<llvm::Type *> fields;
		fields.push_back(
			llvm::ArrayType::get(llvm::PointerType::get(*state.TheContext, 0), vmems.size()));

		this->metadata_struct_type =
			llvm::StructType::create(*state.TheContext, fields, std::string(".meta(") + name + ")");
	}
	return metadata_struct_type;
}

llvm::GlobalVariable *type_class::get_llvm_metadata_object(codegen::state &state) {
	if (metadata_object == nullptr) {
		auto vmems = get_virtual_members(state);
		auto array_type =
			llvm::ArrayType::get(llvm::PointerType::get(*state.TheContext, 0), vmems.size());
		std::vector<llvm::Constant *> vtable;
		for (auto &vmem : vmems) {
			vtable.push_back((llvm::Constant *)state.symbol_table[vmem.get_fqn()].value);
		}
		auto array = llvm::ConstantArray::get(array_type, vtable);
		auto struct_constant =
			llvm::ConstantStruct::get(get_llvm_metadata_struct_type(state), array);

		// state.Builder.SetInsertPoint(&state.init_function->getEntryBlock());
		auto metadataName = std::string(".meta(") + name + ")";
		state.TheModule->getOrInsertGlobal(metadataName, get_llvm_metadata_struct_type(state));
		metadata_object = state.TheModule->getNamedGlobal(metadataName);
		metadata_object->setDSOLocal(true);
		// metadata_object->setInitializer(llvm::ConstantAggregateZero::get(get_llvm_metadata_struct_type(state)));
		metadata_object->setInitializer(struct_constant);
		// metadata_object->setLinkage(llvm::GlobalValue::InternalLinkage);
	}
	return metadata_object;
}

} // namespace catalyst::compiler::codegen
