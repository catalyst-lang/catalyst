// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"

#include "../object_type.hpp"

namespace catalyst::compiler::codegen {

type_iface::type_iface(const std::string &name, std::vector<member> const &members)
	: type_virtual("iface", name, members) {}

type_iface::type_iface(const std::string &name, const std::vector<std::shared_ptr<type_virtual>> &super,
                       std::vector<member> const &members)
	: type_virtual("iface", name, super, members) {}

std::shared_ptr<type_iface> type_iface::unknown() {
	static auto u = std::make_shared<type_iface>("<unknown>", std::vector<member>{});
	return u;
}

std::shared_ptr<type> type::create_iface(const std::string &name,
                                         std::vector<member> const &members) {
	return std::make_shared<type_iface>(name, members);
}

std::shared_ptr<type> type::create_iface(const std::string &name, const std::vector<std::shared_ptr<type_virtual>> &super,
                                         std::vector<member> const &members) {
	return std::make_shared<type_iface>(name, super, members);
}

llvm::Type *type_iface::get_llvm_struct_type(state &state) const {
	if (structType == nullptr) {
		std::vector<llvm::Type *> fields;
		
		
		if (!super.empty()) {
			for (auto const & s : super) {
				fields.push_back(s->get_llvm_struct_type(state));
			}
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

		auto self = const_cast<type_iface *>(this);
		self->structType = llvm::StructType::create(*state.TheContext, fields, name, false);
	}
	return structType;
}

llvm::Type *type_iface::get_llvm_type(state &state) const {
	return llvm::PointerType::get(*state.TheContext, 0);
}

std::string type_iface::get_fqn() const {
	std::string base = "";
	if (!super.empty()) {
		for (auto const & s : super) {
			if (base.empty()) {
				base += ":";
			} else {
				base += ",";
			}
			base += s->name;
		}
	}
	std::string fqn = "iface(" + name + base + "){";
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

bool type_iface::is_valid() const {
	for (const auto &mmbr : members) {
		if (!mmbr.type->is_valid())
			return false;
	}
	for (auto const & s : super){
		if (!s->is_valid()) {
			return false;
		}
	}
	return true;
}

void type_iface::copy_from(type_iface &other) {
	this->name = other.name;
	this->members.clear();
	this->members = other.members;
	this->super = other.super;
}

llvm::Value *type_iface::get_sizeof(catalyst::compiler::codegen::state &state) {
	// Use trick from http://nondot.org/sabre/LLVMNotes/SizeOf-OffsetOf-VariableSizedStructs.txt
	// to mimic a sizeof()

	auto constant = llvm::Constant::getNullValue(get_llvm_type(state)->getPointerTo());
	auto size = state.Builder.CreateConstGEP1_64(get_llvm_struct_type(state), constant, 1, "sizep");
	return state.Builder.CreatePtrToInt(size, llvm::IntegerType::get(*state.TheContext, 64),
	                                    "sizei");
}

llvm::StructType *type_iface::get_llvm_metadata_struct_type(codegen::state &state) {
	if (metadata_struct_type == nullptr) {
		auto vmems = get_virtual_members();
		std::vector<llvm::Type *> fields;
		fields.push_back(
			llvm::ArrayType::get(llvm::PointerType::get(*state.TheContext, 0), vmems.size()));

		this->metadata_struct_type =
			llvm::StructType::create(*state.TheContext, fields, std::string(".meta(") + name + ")");
	}
	return metadata_struct_type;
}

llvm::GlobalVariable *type_iface::get_llvm_metadata_object(codegen::state &state) {
	if (metadata_object == nullptr) {
		auto vmems = get_virtual_members();
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
