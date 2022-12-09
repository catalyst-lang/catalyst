// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <lexy/input_location.hpp>

#include "catalyst/ast/ast.hpp"

namespace catalyst::parser {

template <typename Input>
auto ast_node_get_input_location(const ast_node &positional, const Input &input) {
	return lexy::get_input_location(input, positional.lexeme.begin);
}

template <typename Input>
auto ast_node_get_input_line_annotation(const ast_node &positional, const Input &input) {
	return lexy::get_input_line_annotation(input, ast_node_get_input_location(positional, input), positional.lexeme.end);
}

}
