// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

namespace catalyst::compiler::codegen {
struct type;
}

#include "catalyst/ast/expr.hpp"
#include "codegen.hpp"
#include "llvm/IR/Type.h"
#include <memory>
#include <string>
#include <utility>

namespace catalyst::compiler::codegen {

struct type {
	bool is_valid = true;

	/// Specialization score is a comparable score that specifies the amount of specialization
	/// of this type. For instance, i32 has a higher score than i16.
	int specialization_score = 0;

	virtual ~type() = default;

	type(std::string fqn, bool is_valid = true) : is_valid(is_valid), fqn(std::move(fqn)) {}
	type(std::string fqn, int specialization_score)
		: fqn(std::move(fqn)), specialization_score(specialization_score) {}

	static std::shared_ptr<type> create(const std::string &name = "");
	static std::shared_ptr<type> create(const catalyst::ast::type &ast_type);
	static std::shared_ptr<type> create_function(const std::shared_ptr<type> &return_type);
	static std::shared_ptr<type>
	create_function(const std::shared_ptr<type> &return_type,
	                std::vector<std::shared_ptr<type>> const &parameters);

	virtual llvm::Type *get_llvm_type(codegen::state &state) const = 0;
	virtual llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value,
	                                     std::shared_ptr<type> to);

	bool operator==(const type &other) const;
	bool operator!=(const type &other) const;

	inline bool equals(const type &other) const { return *this == other; }
	inline bool equals(const type* other) const { return *this == *other; }
	inline bool equals(const std::shared_ptr<type> other) const { return *this == *other; }

  private:
	std::string fqn;

  public:
	virtual std::string get_fqn() const;
};

struct type_undefined : type {
	type_undefined() : type("<undefined>", false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
};

struct type_primitive : type {
	type_primitive(std::string fqn, int specialization_score, bool is_signed = true)
		: type(fqn, specialization_score), is_signed(is_signed) {}

	bool is_signed = true;

	virtual llvm::Value *get_llvm_constant_zero(codegen::state &state) const = 0;
	llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value,
	                             std::shared_ptr<type> to) override;
};

struct type_bool : type_primitive {
	type_bool() : type_primitive("bool", 8, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i8 : type_primitive {
	type_i8() : type_primitive("i8", 10) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i16 : type_primitive {
	type_i16() : type_primitive("i16", 11) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i32 : type_primitive {
	type_i32() : type_primitive("i32", 12) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i64 : type_primitive {
	type_i64() : type_primitive("i64", 13) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i128 : type_primitive {
	type_i128() : type_primitive("i128", 14) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_isize : type_primitive {
	type_isize() : type_primitive("isize", 13) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u8 : type_primitive {
	type_u8() : type_primitive("u8", 10, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u16 : type_primitive {
	type_u16() : type_primitive("u16", 11, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u32 : type_primitive {
	type_u32() : type_primitive("u32", 12, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u64 : type_primitive {
	type_u64() : type_primitive("u64", 13, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u128 : type_primitive {
	type_u128() : type_primitive("u128", 14, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_usize : type_primitive {
	type_usize() : type_primitive("usize", 13, false) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_fp16 : type_primitive {
	type_fp16() : type_primitive("fp16", 20) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_fp32 : type_primitive {
	type_fp32() : type_primitive("fp32", 21) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_fp64 : type_primitive {
	type_fp64() : type_primitive("fp64", 22) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_fp128 : type_primitive {
	type_fp128() : type_primitive("fp128", 24) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_fp80 : type_primitive {
	type_fp80() : type_primitive("fp80", 23) {}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_function : type {
	explicit type_function(std::shared_ptr<type> return_type)
		: type("function"), return_type(std::move(return_type)) {}
	type_function(std::shared_ptr<type> return_type,
	              std::vector<std::shared_ptr<type>> const &parameters)
		: type("function"), return_type(std::move(return_type)), parameters(parameters) {}
	std::shared_ptr<type> return_type;
	std::vector<std::shared_ptr<type>> parameters;
	llvm::Type *get_llvm_type(codegen::state &state) const override;

	virtual std::string get_fqn() const override;
};

} // namespace catalyst::compiler::codegen
