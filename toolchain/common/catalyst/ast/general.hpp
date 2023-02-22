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
#include <utility>
#include <variant>
#include <vector>

#include "../../../parser/src/types.hpp"

#include "parser.hpp"

namespace catalyst::ast {

struct expr;
struct statement;
using expr_ptr = std::shared_ptr<struct expr>;
using statement_ptr = std::shared_ptr<struct statement>;

struct ident : parser::ast_node {
	ident(const ident &ident) = default;
	explicit ident(parser::char_type const *begin, parser::char_type const *end, std::string &&name)
		: parser::ast_node(begin, end), name(name) {}
	std::string name;
};

struct type : parser::ast_node {
	explicit type(parser::char_type const *begin, parser::char_type const *end)
		: parser::ast_node(begin, end) {}
	
	virtual ~type() = default;
};

using type_ptr = std::shared_ptr<type>;

struct type_ident : type {
	explicit type_ident(parser::char_type const *begin, parser::char_type const *end, ident const &ident)
		: type(begin, end), ident(ident) {}

	ident ident;
};

struct decl : parser::ast_node {
	decl(const parser::char_type *begin, const parser::char_type *end, ast::ident const &ident)
		: parser::ast_node(begin, end), ident(ident) {}
	ident ident;

	virtual ~decl() = default;
};

struct fn_parameter : parser::ast_node {
	// fn_parameter(parser::char_type *begin, parser::char_type *end, ast::ident &ident,
	//              std::optional<type> &type)
	// 	: parser::ast_node(begin, end), ident(ident), type(type) {}
	ident ident;
	std::optional<type_ptr> type;
};

struct type_function : type {
	explicit type_function(parser::char_type const *begin, parser::char_type const *end, std::vector<ast::fn_parameter> &parameter_list, type_ptr &return_type)
		: parameter_list(parameter_list), return_type(return_type), type(begin, end) {}

	std::vector<ast::fn_parameter> parameter_list;
	type_ptr return_type;
};

struct fn_body_block : parser::ast_node {
	fn_body_block(const parser::char_type *begin, const parser::char_type *end,
	              std::vector<statement_ptr> statements)
		: parser::ast_node(begin, end), statements(statements) {}
	std::vector<statement_ptr> statements;
};

} // namespace catalyst::ast

#include "expr.hpp"

namespace catalyst::ast {

using decl_ptr = std::shared_ptr<decl>;

struct fn_body_expr : parser::ast_node {
	fn_body_expr(expr_ptr expr)
		: parser::ast_node(expr->lexeme.begin, expr->lexeme.end), expr(expr) {}
	expr_ptr expr;
};

using fn_body = std::variant<fn_body_block, fn_body_expr>;

struct decl_fn : decl {
	decl_fn(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident,
	        std::vector<ast::fn_parameter> &parameter_list, std::optional<type_ptr> type,
	        fn_body &body)
		: decl(begin, end, ident), parameter_list(parameter_list), type(type), body(body) {}
	std::vector<ast::fn_parameter> parameter_list;
	std::optional<ast::type_ptr> type = std::nullopt;
	fn_body body;
};

struct decl_var : decl {
	decl_var(const parser::char_type *begin, const parser::char_type *end, ast::ident ident,
	         std::optional<ast::type_ptr> type, std::optional<ast::expr_ptr> expr)
		: decl(begin, end, ident), type(type), expr(expr) {}

	std::optional<ast::type_ptr> type = std::nullopt;
	std::optional<ast::expr_ptr> expr = std::nullopt;
	bool is_const = false;
};

struct decl_const : decl_var {
	decl_const(const parser::char_type *begin, const parser::char_type *end, ast::ident ident,
	           std::optional<ast::type_ptr> type, std::optional<ast::expr_ptr> expr)
		: decl_var(begin, end, ident, type, expr) {
		is_const = true;
	}
};

struct decl_struct : decl {
	decl_struct(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident,
	            std::vector<decl_ptr> declarations)
		: decl(begin, end, ident), declarations(declarations) {}

	std::vector<decl_ptr> declarations;
};

struct decl_class : decl {
	decl_class(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident,
	           std::optional<ast::type_ptr> super, std::vector<decl_ptr> declarations)
		: decl(begin, end, ident), declarations(declarations), super(super) {}

	std::optional<ast::type_ptr> super = std::nullopt;
	std::vector<decl_ptr> declarations;
};

struct translation_unit {
	std::vector<decl_ptr> declarations;
	parser::parser_state_ptr parser_state;
};

} // namespace catalyst::ast
