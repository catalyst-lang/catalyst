//
// Created by basdu on 4-11-2022.
//
#include "compiler.hpp"
#include "../../parser/src/parser.hpp"
#include "codegen/codegen.hpp"
#include <iostream>

using namespace catalyst;

namespace catalyst::compiler {

bool compile(catalyst::ast::translation_unit &tu) {
	codegen::state state;

	ast::expr_ptr expr = std::make_shared<ast::expr_literal_numeric>(
		nullptr, nullptr, 1, 123, std::nullopt, std::nullopt, ast::numeric_classifier::none);

	//parser::report_error(tu.parser_state, "This is an error", *tu.declarations[1], "Here");

	return false;
}

bool compile(const std::string &filename) {
	auto ast = parser::parse_filename(filename);
	if (ast.has_value()) {
		return compile(ast.value());
	} else {
		return false;
	}
}

} // namespace catalyst::compiler