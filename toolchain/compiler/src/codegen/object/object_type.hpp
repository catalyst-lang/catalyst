// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "../type.hpp"
#include "object_type_reference.hpp"

#include <unordered_map>

namespace catalyst::compiler::codegen {

struct type_custom : type {
	type_custom(const std::string &fqn, const std::string &name) : type(fqn), name(name) {}
	virtual ~type_custom() = default;

	std::string name;
	std::vector<member> members;

	llvm::Function *init_function = nullptr;

	virtual llvm::StructType *get_llvm_struct_type(codegen::state &state) const = 0;

	virtual member_locator get_member(const std::string &name);
	virtual member_locator get_member(const type_function *function);

	llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value,
	                             const type &to) const override;

	virtual int get_member_index_in_llvm_struct(member *member) const;
	int get_member_index_in_llvm_struct(const member_locator &member_locator) const;

	void serialize(std::ostream& out) const override;
	static std::shared_ptr<type> deserialize(state &state, std::istream& in);
};

struct type_struct : type_custom {
	explicit type_struct(const std::string &name, std::vector<member> const &members);

	bool is_valid() const override;

	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::StructType *get_llvm_struct_type(codegen::state &state) const override;

	std::string get_fqn() const override;

	void copy_from(type_struct &other);

	llvm::Value *get_sizeof(catalyst::compiler::codegen::state &state) override;

	void serialize(std::ostream& out) const override;
	static std::shared_ptr<type> deserialize(state &state, std::istream& in, const std::string &name, const std::string &init_function_name, const std::vector<member> &members);

  private:
	llvm::StructType *structType = nullptr;
};

struct type_virtual : type_custom {
	type_virtual(const std::string &fqn, const std::string &name,
	             std::vector<member> const &members);
	type_virtual(const std::string &fqn, const std::string &name,
	             const std::vector<object_type_reference<type_virtual>> &super,
	             std::vector<member> const &members);


	std::vector<object_type_reference<type_virtual>> super;

	bool is_assignable_from(const std::shared_ptr<type> &type) const override;

	// virtual members
	virtual std::vector<member_locator> get_virtual_members();
	virtual std::vector<member_locator> get_virtual_members(const std::string &name);
	virtual int get_virtual_member_index(codegen::state &state, const member_locator &member);

	member_locator get_member(const std::string &name) override;
	member_locator get_member(const type_function *function) override;
	int get_member_index_in_llvm_struct(member *member) const override;
	virtual int get_super_index_in_llvm_struct(type_custom *super) const;

	virtual llvm::StructType *get_llvm_metadata_struct_type(codegen::state &state) = 0;
	virtual llvm::GlobalVariable *get_llvm_metadata_object(codegen::state &state) = 0;
	virtual llvm::GlobalVariable *get_llvm_metadata_object(codegen::state &state, type_virtual &mimicking_virtual) = 0;

	void serialize(std::ostream& out) const override;
	static std::shared_ptr<type> deserialize(state &state, std::istream& in, const std::string &name, const std::string &init_function_name, const std::vector<member> &members);
};

struct type_class : type_virtual {
	explicit type_class(const std::string &name, std::vector<member> const &members);
	explicit type_class(const std::string &name,
	                    const std::vector<object_type_reference<type_virtual>> &super,
	                    std::vector<member> const &members);

	bool is_valid() const override;

	static std::shared_ptr<type_class> unknown();

	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::StructType *get_llvm_struct_type(codegen::state &state) const override;

	virtual std::string get_fqn() const override;

	void copy_from(type_class &other);

	virtual llvm::Value *get_sizeof(catalyst::compiler::codegen::state &state) override;

	int get_member_index_in_llvm_struct(member *member) const override;
	int get_super_index_in_llvm_struct(type_custom *super) const override;

	member_locator get_virtual_member_function_that_is_compatible(const type_function *function, const std::string &name);

	llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value,
	                             const type &to) const override;
	llvm::StructType *get_llvm_metadata_struct_type(codegen::state &state) override;
	llvm::GlobalVariable *get_llvm_metadata_object(codegen::state &state) override;
	llvm::GlobalVariable *get_llvm_metadata_object(codegen::state &state, type_virtual &mimicking_virtual) override;

	void serialize(std::ostream& out) const override;
	static std::shared_ptr<type> deserialize(state &state, std::istream& in, const std::string &name, const std::string &init_function_name, const std::vector<object_type_reference<type_virtual>> &super, const std::vector<member> &members);

  private:
	llvm::StructType *structType = nullptr;

	llvm::StructType *metadata_struct_type = nullptr;
	std::unordered_map<type_virtual*, llvm::GlobalVariable*> metadata_objects;

	static llvm::Constant* create_thunk_function(codegen::state &state, llvm::Function * function, const type_virtual &from, const type_virtual &to);
};

struct type_object : type {
	explicit type_object(codegen::state &state, const std::string& type_fqn);
	explicit type_object(codegen::state &state, std::shared_ptr<type_custom> custom_type);
	explicit type_object(object_type_reference<type_custom> custom_type);

	//std::shared_ptr<type_custom> object_type;
	object_type_reference<type_custom> object_type;

	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value,
	                             const type &to) const override;
	bool is_assignable_from(const std::shared_ptr<type> &type) const override;

	std::string get_fqn() const override;

	bool is_valid() const override;

	inline llvm::Value *get_sizeof(codegen::state &state) override {
		return object_type->get_sizeof(state);
	}

	void serialize(std::ostream& out) const override;
	static std::shared_ptr<type> deserialize(state &state, std::istream& in);
};

} // namespace catalyst::compiler::codegen
