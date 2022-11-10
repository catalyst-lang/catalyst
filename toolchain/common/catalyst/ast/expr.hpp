// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "general.hpp"
#include "parser.hpp"
#include <memory>
#include <variant>

namespace catalyst::ast {

struct expr;
using expr_ptr = std::shared_ptr<struct expr>;

struct expr_ident;
struct expr_literal;
struct expr_literal_bool;
struct expr_literal_numeric;
struct expr_binary_arithmetic;
struct expr_binary_logical;
struct expr_unary_arithmetic;
struct expr_call;
struct expr_member_access;

struct expr {
	virtual ~expr() = default;
};

struct expr_ident : expr, ident {
	expr_ident(const ident &ident) : ast::ident(ident) {}
};

struct expr_literal : expr, parser::positional {
	expr_literal(const parser::char_type *begin, const parser::char_type *end)
		: parser::positional(begin, end) {}
};

struct expr_literal_bool : expr_literal {
	expr_literal_bool(const parser::char_type *begin, const parser::char_type *end, bool value)
		: expr_literal(begin, end), value(value) {}

	bool value;
};

// usually by suffixes
enum class numeric_classifier {
	none,
	unsigned_,  // u
	signed_,    // i
	size,       // z
	signed8,    // i8
	unsigned8,  // u8
	signed16,   // i16
	unsigned16, // u16
	signed32,   // i32
	unsigned32, // u32
	signed64,   // i64
	unsigned64, // u64
};

struct expr_literal_numeric : expr_literal {
	expr_literal_numeric(const parser::char_type *begin, const parser::char_type *end, int sign,
	                     int64_t value, std::optional<int64_t> const &fraction,
	                     std::optional<int16_t> const &exponent, numeric_classifier classifier)
		: expr_literal(begin, end), sign(sign), integer(value), fraction(fraction),
		  exponent(exponent), classifier(classifier) {}
	expr_literal_numeric(uint64_t value)
		: sign(1), integer(value), classifier(numeric_classifier::signed64), fraction(std::nullopt),
		  exponent(std::nullopt), expr_literal(nullptr, nullptr) {}

	int sign;
	int64_t integer;
	std::optional<int64_t> fraction;
	std::optional<int16_t> exponent;
	numeric_classifier classifier;
};

struct expr_call : expr {
	expr_ptr lhs;
	std::vector<expr_ptr> parameters;

	expr_call(expr_ptr lhs, std::vector<expr_ptr> params)
		: lhs(std::move(lhs)), parameters(std::move(params)) {}
};

struct expr_member_access : expr {
	expr_ptr lhs;
	expr_ptr rhs;

	expr_member_access(expr_ptr lhs, expr_ptr rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

struct expr_unary_arithmetic : expr {
	enum op_t {
		negate,
		complement,
	} op;
	expr_ptr rhs;

	explicit expr_unary_arithmetic(op_t op, expr_ptr e) : op(op), rhs(std::move(e)) {}
};

struct expr_binary_arithmetic : expr {
	enum op_t {
		plus,
		minus,
		times,
		div,
		pow,
		mod,
		bit_and,
		bit_or,
		bit_xor,
		bit_lsh,
		bit_rsh,
	} op;
	expr_ptr lhs, rhs;

	explicit expr_binary_arithmetic(expr_ptr lhs, op_t op, expr_ptr rhs)
		: op(op), lhs(lhs), rhs(rhs) {}
};

struct expr_binary_logical : expr {
	enum op_t { logical_and, logical_or } op;
	expr_ptr lhs, rhs;

	explicit expr_binary_logical(expr_ptr lhs, op_t op, expr_ptr rhs)
		: op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

} // namespace catalyst::ast
