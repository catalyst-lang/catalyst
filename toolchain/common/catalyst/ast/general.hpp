// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../../../parser/src/types.hpp"

#include "parser.hpp"

namespace catalyst::ast {

struct expr;
struct statement;
struct decl;
using expr_ptr = std::shared_ptr<struct expr>;
using decl_ptr = std::shared_ptr<struct decl>;
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

struct type_qualified_name : type {
	explicit type_qualified_name(parser::char_type const *begin, parser::char_type const *end,
	                    const std::vector<ident> &idents)
		: type(begin, end), idents(idents) {}

	std::vector<ident> idents;

	[[nodiscard]] inline std::string to_string() const {
		std::string ret;
		for (int i = 0; i < idents.size(); i++) {
			if (i > 0) ret += ".";
			ret += idents[i].name;
		}
		return ret;
	}
};

struct type_function_parameter : parser::ast_node {
	std::optional<ident> ident;
	type_ptr type;
};

struct type_function : type {
	explicit type_function(parser::char_type const *begin, parser::char_type const *end,
	                       const std::vector<ast::type_function_parameter> &parameter_list,
	                       type_ptr return_type)
		: type(begin, end), parameter_list(parameter_list), return_type(std::move(return_type)) {}

	std::vector<ast::type_function_parameter> parameter_list;
	type_ptr return_type;
};

struct translation_unit {
	std::vector<decl_ptr> declarations;
	parser::parser_state_ptr parser_state;
};

} // namespace catalyst::ast
