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

enum class decl_classifier {
	public_,
	private_,
	protected_,
	virtual_,
	static_,
	abstract_,
	override_,
};

struct decl : parser::ast_node {
	decl(const parser::char_type *begin, const parser::char_type *end, ast::ident const &ident)
		: parser::ast_node(begin, end), ident(ident) {}
	ident ident;

	std::vector<decl_classifier> classifiers;

	virtual ~decl() = default;
};

struct fn_parameter : parser::ast_node {
	// fn_parameter(parser::char_type *begin, parser::char_type *end, ast::ident &ident,
	//              std::optional<type> &type)
	// 	: parser::ast_node(begin, end), ident(ident), type(type) {}
	ident ident;
	std::optional<type_ptr> type;
};

struct fn_body_block : parser::ast_node {
	fn_body_block(const parser::char_type *begin, const parser::char_type *end,
	              std::vector<statement_ptr> statements)
		: parser::ast_node(begin, end), statements(statements) {}
	std::vector<statement_ptr> statements;
};

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
	        std::optional<fn_body> body)
		: decl(begin, end, ident), parameter_list(parameter_list), type(type), body(body) {}
	std::vector<ast::fn_parameter> parameter_list;
	std::optional<ast::type_ptr> type = std::nullopt;
	std::optional<fn_body> body;
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
	           const std::vector<ast::type_ptr> &super, std::vector<decl_ptr> declarations)
		: decl(begin, end, ident), declarations(declarations), super(super) {}

	std::vector<ast::type_ptr> super;
	std::vector<decl_ptr> declarations;
};

struct decl_ns : decl {
	decl_ns(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident,
	        std::vector<decl_ptr> declarations, bool is_global = false)
		: decl(begin, end, ident), declarations(declarations), is_global(is_global) {}
	std::vector<decl_ptr> declarations;
	bool is_global = false;
};

}