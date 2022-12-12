// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#include "parser.hpp"
#include <memory>
#include <variant>

namespace catalyst::ast {

struct expr : parser::ast_node {
	virtual ~expr() = default;
	using ast_node::ast_node;
};
using expr_ptr = std::shared_ptr<struct expr>;
} // namespace catalyst::ast

#include "general.hpp"

namespace catalyst::ast {

struct expr_ident;
struct expr_literal;
struct expr_literal_bool;
struct expr_literal_numeric;
struct expr_binary_arithmetic;
struct expr_binary_logical;
struct expr_unary_arithmetic;
struct expr_call;
struct expr_member_access;

struct expr_ident : expr {
	explicit expr_ident(const ident &ident)
		: expr(ident.lexeme.begin, ident.lexeme.end), ident(ident) {}
	ident ident;
};

struct expr_literal : expr {
	expr_literal(const parser::char_type *begin, const parser::char_type *end) : expr(begin, end) {}
};

struct expr_literal_bool : expr_literal {
	expr_literal_bool(const parser::char_type *begin, const parser::char_type *end, bool value)
		: expr_literal(begin, end), value(value) {}

	bool value;
};

// usually by suffixes
enum class numeric_classifier {
	none,
	unsigned_,   // u
	signed_,     // i
	size,        // z
	signed8,     // i8
	unsigned8,   // u8
	signed16,    // i16
	unsigned16,  // u16
	signed32,    // i32
	unsigned32,  // u32
	signed64,    // i64
	unsigned64,  // u64
	signed128,   // i64
	unsigned128, // u64
	float_,      // f
	float16,     // f16
	float32,     // f32
	float64,     // f64
	float128,    // f128
	float80,     // f80
};

struct expr_literal_numeric : expr_literal {
	expr_literal_numeric(const parser::char_type *begin, const parser::char_type *end, int sign,
	                     int64_t value, std::optional<int64_t> const &fraction,
	                     std::optional<int16_t> const &exponent, numeric_classifier classifier)
		: expr_literal(begin, end), sign(sign), integer(value), fraction(fraction),
		  exponent(exponent), classifier(classifier) {}
	explicit expr_literal_numeric(uint64_t value)
		: expr_literal(nullptr, nullptr), sign(1), integer(value), fraction(std::nullopt),
		  exponent(std::nullopt), classifier(numeric_classifier::signed64) {}

	int sign;
	int64_t integer;
	std::optional<int64_t> fraction;
	std::optional<int16_t> exponent;
	numeric_classifier classifier;
};

struct expr_call : expr {
	expr_ptr lhs;
	std::vector<expr_ptr> parameters;

	expr_call(const parser::char_type *begin, const parser::char_type *end, expr_ptr lhs,
	          std::vector<expr_ptr> params)
		: expr(begin, end), lhs(std::move(lhs)), parameters(std::move(params)) {}
};

struct expr_member_access : expr {
	expr_ptr lhs;
	expr_ptr rhs;

	expr_member_access(const parser::char_type *begin, expr_ptr lhs, expr_ptr rhs)
		: expr(begin, rhs->lexeme.end), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

struct expr_unary_arithmetic : expr {
	enum op_t {
		negate,
		complement,
	} op;
	expr_ptr rhs;

	explicit expr_unary_arithmetic(op_t op, expr_ptr e)
		: expr(nullptr, nullptr), op(op), rhs(std::move(e)) {}
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
		: expr(lhs->lexeme.end, rhs->lexeme.begin), op(op), lhs(lhs), rhs(rhs) {}
};

struct expr_binary_logical : expr {
	enum op_t { logical_and, logical_or } op;
	expr_ptr lhs, rhs;

	explicit expr_binary_logical(expr_ptr lhs, op_t op, expr_ptr rhs)
		: expr(lhs->lexeme.end, rhs->lexeme.begin), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

struct expr_cast : expr {
	expr_ptr lhs;
	ast::type type;

	explicit expr_cast(const parser::char_type *begin, const parser::char_type *end, expr_ptr lhs,
	                   ast::type type)
		: expr(begin, end), lhs(lhs), type(type) {}
};

struct expr_assignment : expr {
	enum op_t {
		assign,
	} op;
	expr_ptr lhs, rhs;

	explicit expr_assignment(expr_ptr lhs, op_t op, expr_ptr rhs)
		: expr(lhs->lexeme.end, rhs->lexeme.begin), op(op), lhs(lhs), rhs(rhs) {}
};

} // namespace catalyst::ast
