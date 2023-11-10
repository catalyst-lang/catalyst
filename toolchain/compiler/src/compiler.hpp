// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <sstream>
#include <string>
#include <memory>

#include "catalyst/ast/ast.hpp"

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

struct compile_session {
	bool is_successful = false;
	bool is_runnable = false;
	std::shared_ptr<void> state;
	std::string result_type_name;

	static compile_session create_failed() {
		compile_session s;
		s.is_successful = false;
		s.is_runnable = false;
		return s;
	}
};

bool compile_file(compile_session&, const std::string &filename, options);
bool compile_string(compile_session&, const std::string &string, options);
compile_session compile_string(const std::string &string, options);
bool compile(compile_session&, catalyst::ast::translation_unit &tu, options);
bool create_meta(const compile_session &result, std::ostream& out);
void compiler_import_bundle(compile_session& session, const std::string &filename, options options);

void compiler_debug_print(compile_session &);

codegen::state &get_state(const compile_session &result);

std::string get_default_target_triple();

template<typename T>
T run(const compile_session &result);

} // namespace catalyst
