// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <lexy/input_location.hpp>

#include "catalyst/ast.hpp"

namespace catalyst::parser {

template <typename Input>
auto positional_get_input_location(const positional &positional, const Input &input) {
	return lexy::get_input_location(input, positional.lexeme.begin);
}

template <typename Input>
auto positional_get_input_line_annotation(const positional &positional, const Input &input) {
	return lexy::get_input_line_annotation(input, positional_get_input_location(positional, input), positional.lexeme.end);
}

}
