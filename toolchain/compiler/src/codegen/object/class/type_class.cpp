// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "catalyst/rtti.hpp"

#include "../object_type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace catalyst::compiler::codegen {

type_class::type_class(const std::string &name, std::vector<member> const &members)
	: type_virtual("class", name, members) {}

type_class::type_class(const std::string &name, const std::vector<object_type_reference<type_virtual>> &super,
                       std::vector<member> const &members)
	: type_virtual("class", name, super, members) {}

std::shared_ptr<type_class> type_class::unknown() {
	static auto u = std::make_shared<type_class>("<unknown>", std::vector<member>{});
	return u;
}

std::shared_ptr<type> type::create_class(const std::string &name,
                                         std::vector<member> const &members) {
	return std::make_shared<type_class>(name, members);
}

std::shared_ptr<type> type::create_class(const std::string &name, const std::vector<object_type_reference<type_virtual>> &super,
                                         std::vector<member> const &members) {
	return std::make_shared<type_class>(name, super, members);
}

llvm::StructType *type_class::get_llvm_struct_type(state &state) const {
	if (structType == nullptr) {
		std::vector<llvm::Type *> fields;
		
		// Metatdata
		fields.push_back(llvm::PointerType::get(*state.TheContext, 0));

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

		auto self = const_cast<type_class *>(this);
		self->structType = llvm::StructType::create(*state.TheContext, fields, name, false);
	}
	return structType;
}

llvm::Type *type_class::get_llvm_type(state &state) const {
	return llvm::PointerType::get(*state.TheContext, 0);
}

std::string type_class::get_fqn() const {
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
	for (auto const & s : super){
		if (!s->is_valid()) {
			return false;
		}
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

llvm::StructType *type_class::get_llvm_metadata_struct_type(codegen::state &state) {
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

member_locator type_class::get_virtual_member_function_that_is_compatible(const type_function *function, const std::string &name) {
	auto vmems = this->get_virtual_members(name);

	for (auto const &vmem : vmems) {
		if (*vmem.member->type == *function) {
			return vmem;
		}
	}
	
	return member_locator::invalid();
}

llvm::GlobalVariable *type_class::get_llvm_metadata_object(codegen::state &state) {
	return get_llvm_metadata_object(state, *this);
}

llvm::GlobalVariable *type_class::get_llvm_metadata_object(codegen::state &state, type_virtual &mimicking_virtual) {
	if (!metadata_objects.contains(&mimicking_virtual)) {
		auto vmems = mimicking_virtual.get_virtual_members();
		auto array_type =
			llvm::ArrayType::get(llvm::PointerType::get(*state.TheContext, 0), vmems.size());
		std::vector<llvm::Constant *> vtable;
		for (const auto &vmem : vmems) {
			member_locator myvmem = isa<type_function>(vmem.member->type) ? 
				get_virtual_member_function_that_is_compatible(std::static_pointer_cast<type_function>(vmem.member->type).get(), vmem.member->name) : 
				get_member(vmem.member->name);
			
			if (!myvmem.is_valid()) {
				std::string err = "Could not find virtual member " + mimicking_virtual.name + "." + vmem.member->name + " in " + this->name;
				state.report_message(report_type::error, err, vmem.member->decl.get());
			} else {
				if (*myvmem.residence != mimicking_virtual && mimicking_virtual != *this) {
					auto const & residence = *myvmem.residence;
					vtable.push_back(create_thunk_function(state, (llvm::Function *)state.symbol_table[myvmem.get_fqn()].value, mimicking_virtual, dynamic_cast<const type_virtual&>(residence)));
				} else {
					vtable.push_back((llvm::Constant *)state.symbol_table[myvmem.get_fqn()].value);
				}
			}
		}
		auto array = llvm::ConstantArray::get(array_type, vtable);
		auto struct_constant =
			llvm::ConstantStruct::get(mimicking_virtual.get_llvm_metadata_struct_type(state), array);

		auto metadataName = std::string(".meta(") + mimicking_virtual.name + (*this != mimicking_virtual ? " in " + name : "") + ")";
		state.TheModule->getOrInsertGlobal(metadataName, mimicking_virtual.get_llvm_metadata_struct_type(state));
		llvm::GlobalVariable *metadata_object = state.TheModule->getNamedGlobal(metadataName);
		metadata_object->setDSOLocal(true);
		metadata_object->setInitializer(struct_constant);
		metadata_objects[&mimicking_virtual] = metadata_object;
		return metadata_object;
	}
	return metadata_objects[&mimicking_virtual];
}

int type_class::get_member_index_in_llvm_struct(member *member) const {
	// add 1 for the metadata ptr
	return 1 + type_virtual::get_member_index_in_llvm_struct(member);
}

int type_class::get_super_index_in_llvm_struct(type_custom *super) const {
	auto ret = type_virtual::get_super_index_in_llvm_struct(super);
	if (ret >= 0) {
		return 1 + ret;
	} else {
		return -1;
	}
}

llvm::Value *type_class::cast_llvm_value(codegen::state &state, llvm::Value *value,
                                          const type &to) const {
	if (!isa<type_class>(to)) {
		return nullptr;
	}

	auto to_class = (type_class*)&to;

	if (this == to_class) return value;

	// null check
	auto *notNullBB = llvm::BasicBlock::Create(*state.TheContext, "is_not_null", state.current_function);
	auto *nullBB = llvm::BasicBlock::Create(*state.TheContext, "is_null", state.current_function);
	auto *mergeBB = llvm::BasicBlock::Create(*state.TheContext, "null_check_merge", state.current_function);

	auto* nullValue = llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(value->getType()));
	auto* null_check = state.Builder.CreateICmpEQ(value, nullValue, "null_check");
	state.Builder.CreateCondBr(null_check, nullBB, notNullBB);

	state.Builder.SetInsertPoint(nullBB);
	state.Builder.CreateBr(mergeBB);

	state.Builder.SetInsertPoint(notNullBB);
	llvm::Value *casted = nullptr;
	if (int index = this->get_super_index_in_llvm_struct(to_class); index >= 0) {
		casted = state.Builder.CreateStructGEP(this->get_llvm_struct_type(state),
												value, index, to_class->name + "_ptr");
	} else {
		for (auto const &s : this->super) {
			if (to_class->is_assignable_from(s.get())) {
				value = state.Builder.CreateStructGEP(
					this->get_llvm_struct_type(state), value,
					this->get_super_index_in_llvm_struct(s.get().get()),
					s->name + "_ptr");
				casted = s->cast_llvm_value(state, value, to);
				break;
			}
		}
	}
	// update notNullBB, because we might be completely somewhere else now because of above function calls
	notNullBB = state.Builder.GetInsertBlock();

	state.Builder.CreateBr(mergeBB);
	state.Builder.SetInsertPoint(mergeBB);
	auto* phi = state.Builder.CreatePHI(llvm::PointerType::get(*state.TheContext, 0), 2, "casted");
	phi->addIncoming(casted, notNullBB);
	phi->addIncoming(nullValue, nullBB);
	
	return phi;
}

llvm::Constant* type_class::create_thunk_function(codegen::state &state, llvm::Function * function, const type_virtual &from, const type_virtual &to) {
	// See if we already have a thunk function in this module
	std::string thunk_name = std::string(function->getName()) + "#thunk:" + from.name;
	if (state.TheModule->getFunction(thunk_name)) {
		return state.TheModule->getFunction(thunk_name);
	}

	// create a new llvm::Function that has the same signature as the function we are thunking
	auto *thunk_function = llvm::Function::Create(function->getFunctionType(), llvm::Function::ExternalLinkage, thunk_name, *state.TheModule);

	// create a new llvm::BasicBlock for the function
	auto *BB = llvm::BasicBlock::Create(*state.TheContext, "entry", thunk_function);
	auto *currentInsertPoint = state.Builder.GetInsertBlock();
	state.Builder.SetInsertPoint(BB);

	std::vector<llvm::Value *> args;
	for (auto &arg : thunk_function->args()) {
		args.push_back(&arg);
	}

	// adjust the address of the first argument (the 'this') from a pointer to type 'from' to pointer of type 'to'.
	auto *this_ = args[0];
	this_->setName("this");
	auto intermediate_to = &to;
	while (*intermediate_to != from && !intermediate_to->super.empty()) {
		for (auto const &s : intermediate_to->super) {
			if (from.is_assignable_from(s.get())) {
				auto nullConstant = llvm::Constant::getNullValue(intermediate_to->get_llvm_type(state)->getPointerTo());
				int index = intermediate_to->get_super_index_in_llvm_struct(s.get().get());

				// Get the offset to from in the parent class struct
				auto* offsetGepInst = state.Builder.CreateStructGEP(intermediate_to->get_llvm_struct_type(state), nullConstant, index, "super_offset");
				auto ptrSize = (unsigned int)state.TheModule->getDataLayout().getTypeAllocSizeInBits(llvm::PointerType::get(*state.TheContext, 0));
				auto ptrIntType = llvm::IntegerType::getIntNTy(*state.TheContext, ptrSize);
				auto *castedPtrInt = state.Builder.CreatePtrToInt(offsetGepInst, ptrIntType);
				auto* negOffset = state.Builder.CreateNeg(castedPtrInt, "neg_offset");
				auto *castedIntPtr = state.Builder.CreateIntToPtr(negOffset, llvm::PointerType::get(*state.TheContext, 0));

				// Subtract the offset from the pointer this_
				this_ = state.Builder.CreateGEP(llvm::Type::getInt8Ty(*state.TheContext), this_, castedIntPtr, "offsetted_this");
				intermediate_to = s.get().get();
				break;
			}
		}
	}
	args[0] = this_;

	// generate a call to the original function with the arguments from the thunk function
	auto *ret = state.Builder.CreateCall(function, args, "thunked_call");
	state.Builder.CreateRet(ret);

	state.Builder.SetInsertPoint(currentInsertPoint);

	return thunk_function;
}

} // namespace catalyst::compiler::codegen
