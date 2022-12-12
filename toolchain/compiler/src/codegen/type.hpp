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
	virtual llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value, type* to);

	virtual llvm::Value *create_add(codegen::state &state, llvm::Value *value,
	                                std::shared_ptr<type> rhs_type, llvm::Value *rhs);

	virtual bool is_assignable_from(const std::shared_ptr<type> &type) const;

	bool operator==(const type &other) const;
	bool operator!=(const type &other) const;

	inline bool equals(const type &other) const { return *this == other; }
	inline bool equals(const type *other) const { return *this == *other; }
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
	int bits = 0;
	bool is_float = false;

	virtual llvm::Value *get_llvm_constant_zero(codegen::state &state) const = 0;
	llvm::Value *cast_llvm_value(codegen::state &state, llvm::Value *value, type *to) override;
	bool is_assignable_from(const std::shared_ptr<type> &type) const override;
	llvm::Value *create_add(codegen::state &state, llvm::Value *value,
	                        std::shared_ptr<type> rhs_type, llvm::Value *rhs) override;
};

struct type_bool : type_primitive {
	type_bool() : type_primitive("bool", 8, false) {
		bits = 1;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i8 : type_primitive {
	type_i8() : type_primitive("i8", 10) {
		bits = 8;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i16 : type_primitive {
	type_i16() : type_primitive("i16", 11) {
		bits = 16;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i32 : type_primitive {
	type_i32() : type_primitive("i32", 12) {
		bits = 32;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i64 : type_primitive {
	type_i64() : type_primitive("i64", 13) {
		bits = 64;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_i128 : type_primitive {
	type_i128() : type_primitive("i128", 14) {
		bits = 128;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_isize : type_primitive {
	type_isize() : type_primitive("isize", 13) {
		bits = 64;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u8 : type_primitive {
	type_u8() : type_primitive("u8", 10, false) {
		bits = 8;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u16 : type_primitive {
	type_u16() : type_primitive("u16", 11, false) {
		bits = 16;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u32 : type_primitive {
	type_u32() : type_primitive("u32", 12, false) {
		bits = 32;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u64 : type_primitive {
	type_u64() : type_primitive("u64", 13, false) {
		bits = 64;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_u128 : type_primitive {
	type_u128() : type_primitive("u128", 14, false) {
		bits = 128;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_usize : type_primitive {
	type_usize() : type_primitive("usize", 13, false) {
		bits = 64;
		is_float = false;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_f16 : type_primitive {
	type_f16() : type_primitive("f16", 20) {
		bits = 16;
		is_float = true;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_f32 : type_primitive {
	type_f32() : type_primitive("f32", 21) {
		bits = 32;
		is_float = true;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_f64 : type_primitive {
	type_f64() : type_primitive("f64", 22) {
		bits = 64;
		is_float = true;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_f128 : type_primitive {
	type_f128() : type_primitive("f128", 24) {
		bits = 128;
		is_float = true;
	}
	llvm::Type *get_llvm_type(codegen::state &state) const override;
	llvm::Value *get_llvm_constant_zero(codegen::state &state) const override;
};

struct type_f80 : type_primitive {
	type_f80() : type_primitive("f80", 23) {
		bits = 80;
		is_float = true;
	}
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
