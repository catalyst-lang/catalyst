//
// Created by basdu on 4-11-2022.
//
#include <iostream>
#include "compiler.hpp"
#include "../../parser/src/parser.hpp"
#include "codegen/codegen.hpp"
#include "codegen/expr.hpp"

using namespace catalyst;

namespace catalyst::compiler {

bool compile(catalyst::ast::translation_unit &tu) {
	codegen::state state;
	state.translation_unit = &tu;
	state.TheModule = std::make_unique<llvm::Module>(tu.parser_state->filename, state.TheContext);

	auto v = codegen::codegen(state);
	printf("Read module:\n");
	state.TheModule->print(llvm::outs(), nullptr);
	printf("\n");

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