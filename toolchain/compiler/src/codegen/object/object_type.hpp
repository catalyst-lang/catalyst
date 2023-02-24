// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

namespace catalyst::compiler::codegen {
struct type_custom;
}

#include "../type.hpp"

namespace catalyst::compiler::codegen {

struct type_custom : type {
	type_custom(std::string fqn, std::string name) : type(fqn), name(name) {}

	std::string name;
	std::vector<member> members;

	llvm::Function *init_function = nullptr;

	virtual llvm::Type *get_llvm_struct_type(codegen::state &state) const = 0;

	virtual member_locator get_member(const std::string &name);
	virtual member_locator get_member(const type_function *function);

	virtual int get_member_index_in_llvm_struct(member *member);
	int get_member_index_in_llvm_struct(const member_locator &member_locator);
};

struct type_struct : type_custom {
	explicit type_struct(const std::string &name, std::vector<member> const &members);

	bool is_valid() override;

	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Type *get_llvm_struct_type(codegen::state &state) const override;

	virtual std::string get_fqn() const override;

	void copy_from(type_struct &other);

	inline virtual llvm::Value *get_sizeof(catalyst::compiler::codegen::state &state) override;

  private:
	llvm::StructType *structType = nullptr;
};

struct type_class : type_custom {
	explicit type_class(const std::string &name, std::vector<member> const &members);
	explicit type_class(const std::string &name, std::shared_ptr<type_class> super,
	                    std::vector<member> const &members);

	bool is_valid() override;

	static std::shared_ptr<type_class> unknown();

	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Type *get_llvm_struct_type(codegen::state &state) const override;

	std::vector<member_locator> get_virtual_members(codegen::state &state);
	int get_virtual_member_index(codegen::state &state, const member_locator& member);

	llvm::StructType *get_llvm_metadata_struct_type(codegen::state &state);
	llvm::GlobalVariable *get_llvm_metadata_object(codegen::state &state);

	virtual std::string get_fqn() const override;

	void copy_from(type_class &other);

	inline virtual llvm::Value *get_sizeof(catalyst::compiler::codegen::state &state) override;

	std::shared_ptr<type_class> super;

	member_locator get_member(const std::string &name) override;
	member_locator get_member(const type_function *function) override;
	int get_member_index_in_llvm_struct(member *member) override;

  private:
	llvm::StructType *structType = nullptr;

	llvm::StructType *metadata_struct_type = nullptr;
	llvm::GlobalVariable *metadata_object = nullptr;
};

struct type_object : type {
	explicit type_object(std::shared_ptr<type_custom> object_type);

	std::shared_ptr<type_custom> object_type;

	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value,
	                             const type &to) const override;
	bool is_assignable_from(const std::shared_ptr<type> &type) const override;

	virtual std::string get_fqn() const override;

	bool is_valid() override;

	inline llvm::Value *get_sizeof(codegen::state &state) override {
		return object_type->get_sizeof(state);
	}
};

} // namespace catalyst::compiler::codegen
