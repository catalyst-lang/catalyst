// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <iostream>

#include "catalyst/ast/ast.hpp"
#include "catalyst/version.hpp"
#include "types.hpp"

#include "state.hpp"

namespace catalyst::parser {

constexpr struct {
	int major = 0, minor = 0, patch = 1;

	std::string string() const {
		std::stringstream ss;
		ss << major << '.' << minor << '.' << patch;
		return ss.str();
	}
} version;

std::optional<ast::translation_unit> parse_string(const std::string &string);
std::optional<catalyst::ast::translation_unit> parse_filename(const std::string &filename);

enum class report_type { error, warning, info, debug, help };

void report_message(report_type type, const std::string &error_title, std::ostream& os = std::cout);
void report_message(report_type type, parser_state_ptr parser_state,
                    const std::string &message_title, const parser::ast_node &positional,
                    const std::string &message_positional_title, std::ostream& os = std::cout);

} // namespace catalyst::parser
