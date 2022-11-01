#pragma once

#include "ast.hpp"
#include <optional>
#include <sstream>
#include <string>

#include "types.hpp"
#include "catalyst/version.hpp"
#include "ast.hpp"

namespace catalyst::parser {

constexpr struct {
	int major = 0, minor = 0, patch = 1;

	std::string string() const {
		std::stringstream ss;
		ss << major << '.' << minor << '.' << patch;
		return ss.str();
	}
} version;

std::optional<catalyst::ast::translation_unit> parse(const std::string &string);

}
