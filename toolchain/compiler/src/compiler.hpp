// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "catalyst/ast/ast.hpp"
#include <sstream>
#include <string>
#include <memory>

namespace catalyst::compiler {

constexpr struct {
	int major = 0, minor = 0, patch = 1;

	std::string string() const {
		std::stringstream ss;
		ss << major << '.' << minor << '.' << patch;
		return ss.str();
	}
} version;

struct options {
	int optimizer_level = 0;
};

struct compile_result {
	bool is_successful;
	std::shared_ptr<void> state;

	static compile_result create_failed() {
		compile_result s;
		s.is_successful = false;
		return s;
	}
};

compile_result compile(const std::string &string, options);
compile_result compile(catalyst::ast::translation_unit &tu, options);

uint64_t run(compile_result &);

} // namespace catalyst
