// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "lexy/encoding.hpp"
#include "lexy/input/buffer.hpp"
#include "lexy/input/string_input.hpp"

namespace catalyst::parser {

using parser_input_buffer = lexy::buffer<lexy::deduce_encoding<parser::char_type>, void>;

struct parser_state {
	parser_input_buffer input;
	std::string filename;
};

} // namespace catalyst
