#pragma once

#include "catalyst/ast/ast.hpp"
#include <optional>
#include <sstream>
#include <string>

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

void report_error(const std::string &error_title);
void report_error(parser_state_ptr parser_state, const std::string &error_title, const parser::positional &positional,
                  const std::string &error_positional_title);

}