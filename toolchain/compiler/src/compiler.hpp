// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "catalyst/ast/ast.hpp"
#include <sstream>
#include <string>
#include <memory>

namespace catalyst::compiler {

namespace codegen {
	struct state;
}

constexpr struct {
	int major = 0, minor = 0, patch = 1;

	[[nodiscard]] std::string string() const {
		std::stringstream ss;
		ss << major << '.' << minor << '.' << patch;
		return ss.str();
	}
} version;

struct options {
	int optimizer_level = 0;
};

struct compile_result {
	bool is_successful = false;
	bool is_runnable = false;
	std::shared_ptr<void> state;
	std::string result_type_name;

	static compile_result create_failed() {
		compile_result s;
		s.is_successful = false;
		return s;
	}
};

compile_result compile_file(const std::string &filename, options);
compile_result compile_string(const std::string &string, options);
compile_result compile(catalyst::ast::translation_unit &tu, options);

void compiler_debug_print(compile_result &);

codegen::state &get_state(const compile_result &result);

template<typename T>
T run(const compile_result &result);

} // namespace catalyst
