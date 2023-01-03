// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <utility>

#include "expr.hpp"
#include "general.hpp"
#include "parser.hpp"

namespace catalyst::ast {

struct statement;
using statement_ptr = std::shared_ptr<struct statement>;

struct statement_var;
struct statement_const;
struct statement_expr;
struct statement_return;
struct statement_if;
struct statement_for;

struct statement : parser::ast_node {
	virtual ~statement() = default;
	using ast_node::ast_node;
};

struct statement_decl : statement {
	statement_decl(decl_ptr decl)
		: statement(decl->lexeme.begin, decl->lexeme.end), decl(decl) {}

	decl_ptr decl;
};

struct statement_expr : statement {
	statement_expr(expr_ptr expr) : statement(expr->lexeme.begin, expr->lexeme.end), expr(expr) {}

	expr_ptr expr;
};

struct statement_return : statement {
	statement_return(const parser::char_type *begin, const parser::char_type *end, expr_ptr expr)
		: statement(begin, end), expr(expr) {}
	expr_ptr expr;
};

struct statement_block : statement {
	statement_block(const parser::char_type *begin, const parser::char_type *end,
	                const std::vector<statement_ptr> &statements)
		: statement(begin, end), statements(statements) {}
	std::vector<statement_ptr> statements;
};

struct statement_if : statement {
	statement_if(const parser::char_type *begin, const parser::char_type *end, expr_ptr cond,
	             statement_ptr then, std::optional<statement_ptr> else_)
		: statement(begin, end), cond(cond), then(then), else_(else_) {}

	expr_ptr cond;
	statement_ptr then;
	std::optional<statement_ptr> else_;
};

struct statement_for : statement {
	statement_for(const parser::char_type *begin, const parser::char_type *end, ident ident,
	              expr_ptr expr_start, expr_ptr expr_end, expr_ptr expr_step, statement_ptr body)
		: statement(begin, end), ident(ident), expr_start(expr_start), expr_end(expr_end),
		  expr_step(expr_step), body(body) {}

	statement_for(const parser::char_type *begin, const parser::char_type *end, ident ident,
	              expr_ptr expr_start, expr_ptr expr_end, statement_ptr body)
		: statement(begin, end), ident(ident), expr_start(expr_start), expr_end(expr_end),
		  body(body) {
		expr_step = std::make_shared<ast::expr_literal_numeric>(1);
	}

	ident ident;
	expr_ptr expr_start;
	expr_ptr expr_end;
	expr_ptr expr_step;
	statement_ptr body;
};

} // namespace catalyst::ast
