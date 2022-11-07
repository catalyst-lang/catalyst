// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "../../../parser/src/types.hpp"

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

struct parser_state;
using parser_state_ptr = std::shared_ptr<parser_state>;

} // namespace catalyst::parser
