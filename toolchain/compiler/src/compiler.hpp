// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "catalyst/ast/ast.hpp"
#include <sstream>
#include <string>

namespace catalyst::compiler {

constexpr struct {
	int major = 0, minor = 0, patch = 1;

	std::string string() const {
		std::stringstream ss;
		ss << major << '.' << minor << '.' << patch;
		return ss.str();
	}
} version;

bool compile(const std::string &string);
bool compile(catalyst::ast::translation_unit &tu);

} // namespace catalyst
