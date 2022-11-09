// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <utility>

#include "expr.hpp"
#include "general.hpp"

namespace catalyst::ast {

struct statement;
using statement_ptr = std::shared_ptr<struct statement>;

struct statement_var;
struct statement_const;
struct statement_expr;
struct statement_return;
struct statement_if;
struct statement_for;

struct statement {
	virtual ~statement() = default;
};

struct statement_var : statement {
	statement_var(ident ident, std::optional<type> type, std::optional<expr_ptr> expr)
		: ident(ident), type(std::move(type)), expr(std::move(expr)) {}

	ident ident;
	std::optional<type> type;
	std::optional<expr_ptr> expr;
};

struct statement_const : statement {
	statement_const(ident ident, std::optional<type> type, std::optional<expr_ptr> expr)
		: ident(ident), type(std::move(type)), expr(std::move(expr)) {}

	ident ident;
	std::optional<type> type;
	std::optional<expr_ptr> expr;
};

struct statement_expr : statement {
	statement_expr(expr_ptr expr) : expr(expr) {}

	expr_ptr expr;
};

struct statement_return : statement {
	statement_return(expr_ptr expr) : expr(expr) {}
	expr_ptr expr;
};

struct statement_if : statement {
	statement_if(expr_ptr cond, statement_ptr then, std::optional<statement_ptr> else_)
		: cond(cond), then(then), else_(else_) {}

	expr_ptr cond;
	statement_ptr then;
	std::optional<statement_ptr> else_;
};

struct statement_for : statement {
	statement_for(ident ident, expr_ptr start, expr_ptr end, expr_ptr step, statement_ptr body)
		: ident(ident), start(start), end(end), step(step), body(body) {}

	ident ident;
	expr_ptr start;
	expr_ptr end;
	expr_ptr step;
	statement_ptr body;
};

} // namespace catalyst::ast
