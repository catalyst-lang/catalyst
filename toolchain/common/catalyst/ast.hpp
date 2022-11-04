// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../../parser/src/types.hpp"

namespace catalyst::parser {
struct lexeme {
	const parser::char_type *begin;
	const parser::char_type *end;
	explicit lexeme(const parser::char_type *begin, const parser::char_type *end)
		: begin(begin), end(end) {}
	// lexeme(const parser::char_type *begin, const parser::char_type *end) : begin(begin), end(end)
	// {} lexeme(const lexeme &n) : begin(n.begin), end(n.end) { }
};

struct positional {
	parser::lexeme lexeme;

	positional(const parser::char_type *begin, const parser::char_type *end) : lexeme(begin, end) {}
};
} // namespace catalyst::parser

namespace catalyst::ast {

struct ident : parser::positional {
	ident(const ident &ident) : parser::positional(ident), name(ident.name) {}
	explicit ident(parser::char_type *begin, parser::char_type *end, std::string &&name)
		: name(name), parser::positional(begin, end) {}
	std::string name;
};

struct type : parser::positional {
	explicit type(parser::char_type *begin, parser::char_type *end, ident &ident)
		: ident(ident), parser::positional(begin, end) {}
	ident ident;
};

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
	size,       // s
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
	                     int64_t value, std::optional<int64_t> &fraction,
	                     std::optional<int16_t> &exponent, numeric_classifier classifier)
		: expr_literal(begin, end), sign(sign), integer(value), fraction(fraction),
		  exponent(exponent), classifier(classifier) {}

	int sign;
	int64_t integer;
	std::optional<int64_t> fraction;
	std::optional<int16_t> exponent;
	numeric_classifier classifier;
};

using expr_ptr = std::shared_ptr<struct expr>;

struct expr_call : expr {
	expr_ptr lhs;
	std::vector<expr_ptr> parameters;

	expr_call(expr_ptr lhs, std::vector<expr_ptr> &params) : lhs(lhs), parameters(params) {}
};

struct expr_member_access : expr {
	expr_ptr lhs;
	expr_ptr rhs;

	expr_member_access(expr_ptr lhs, expr_ptr rhs) : lhs(lhs), rhs(rhs) {}
};

struct expr_unary_arithmetic : expr {
	enum op_t {
		negate,
		complement,
	} op;
	expr_ptr rhs;

	explicit expr_unary_arithmetic(op_t op, expr_ptr e) : op(op), rhs(e) {}
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
	enum op_t {
		logical_and, 
		logical_or
	} op;
	expr_ptr lhs, rhs;

	explicit expr_binary_logical(expr_ptr lhs, op_t op, expr_ptr rhs)
		: op(op), lhs(lhs), rhs(rhs) {}
};

// using expr = std::variant<expr_literal, expr_ident, expr_binary_arithmetic,
// expr_unary_arithmetic>;

struct statement_var {
	ident ident;
	std::optional<type> type;
	std::optional<expr_ptr> expr;
};

struct statement_const {
	ident ident;
	std::optional<type> type;
	std::optional<expr_ptr> expr;
};

struct statement_expr {
	expr_ptr expr;
};

using statement = std::variant<statement_var, statement_const, statement_expr>;

struct decl : parser::positional {
	decl(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident)
		: parser::positional(begin, end), ident(ident) {}
	ident ident;
};

struct fn_parameter : parser::positional {
	// fn_parameter(parser::char_type *begin, parser::char_type *end, ast::ident &ident,
	//              std::optional<type> &type)
	// 	: parser::positional(begin, end), ident(ident), type(type) {}
	ident ident;
	std::optional<type> type;
};

struct fn_body_block {
	std::vector<statement> statements;
};

struct fn_body_expr {
	expr_ptr expr;
};

using fn_body = std::variant<fn_body_block, fn_body_expr>;

struct decl_fn : decl {
	decl_fn(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident,
	        std::vector<ast::fn_parameter> &parameter_list, fn_body &body)
		: decl(begin, end, ident), parameter_list(parameter_list), body(body) {}
	std::vector<ast::fn_parameter> parameter_list;
	fn_body body;
};

using decl_ptr = std::shared_ptr<decl>;

struct translation_unit {
	std::vector<decl_ptr> declarations;
};

} // namespace catalyst::ast
