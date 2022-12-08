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

struct ident : parser::positional {
	ident(const ident &ident) = default;
	explicit ident(parser::char_type const *begin, parser::char_type const *end, std::string &&name)
		: parser::positional(begin, end), name(name) {}
	std::string name;
};

struct type : parser::positional {
	explicit type(parser::char_type const *begin, parser::char_type const *end, ident const &ident)
		: parser::positional(begin, end), ident(ident) {}
	ident ident;
};

struct decl : parser::positional {
	decl(const parser::char_type *begin, const parser::char_type *end, ast::ident const &ident)
		: parser::positional(begin, end), ident(ident) {}
	ident ident;

	virtual ~decl() = default;
};

struct fn_parameter : parser::positional {
	// fn_parameter(parser::char_type *begin, parser::char_type *end, ast::ident &ident,
	//              std::optional<type> &type)
	// 	: parser::positional(begin, end), ident(ident), type(type) {}
	ident ident;
	std::optional<type> type;
};

struct fn_body_block {
	std::vector<statement_ptr> statements;
};

struct fn_body_expr {
	expr_ptr expr;
};

using fn_body = std::variant<fn_body_block, fn_body_expr>;

struct decl_fn : decl {
	decl_fn(const parser::char_type *begin, const parser::char_type *end, ast::ident &ident,
	        std::vector<ast::fn_parameter> &parameter_list, std::optional<ast::type> type,
	        fn_body &body)
		: decl(begin, end, ident), parameter_list(parameter_list), type(type), body(body) {}
	std::vector<ast::fn_parameter> parameter_list;
	std::optional<ast::type> type;
	fn_body body;
};

using decl_ptr = std::shared_ptr<decl>;

struct translation_unit {
	std::vector<decl_ptr> declarations;
	parser::parser_state_ptr parser_state;
};

} // namespace catalyst::ast
