#pragma once

#include "catalyst/ast/ast.hpp"
#include <optional>
#include <sstream>
#include <string>

#include "catalyst/ast/ast.hpp"
#include "catalyst/version.hpp"
#include "types.hpp"

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

}
