// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iostream>
#include "codegen.hpp"
#include "decl.hpp"
#include "../../../parser/src/parser.hpp"

namespace catalyst::compiler::codegen {

void state::report_error(const std::string &error) {
	parser::report_error(error);
}

void state::report_error(const std::string &error, const parser::positional &positional,
                         const std::string &pos_comment) const {
	parser::report_error(this->translation_unit->parser_state, error, positional, pos_comment);
}

llvm::Value *codegen(codegen::state &state, ast::translation_unit &tu) {
	// fill prototypes
	int pass_n = 0;
	int pass_changes = 1;
	while (pass_changes > 0) {
		pass_changes = 0;
		for (auto decl : tu.declarations) {
			pass_changes += proto_pass(state, pass_n, decl);
		}
		pass_n++;
	}

	for (const auto &[k, v] : state.symbol_table)
	{
		if (v.type == nullptr || !v.type->is_valid) {
			state.report_error("No type has been defined and can't be inferred", *v.get_positional());
		}
		std::cout << k << ": " << v.type->get_fqn() << std::endl;
	}

	for(auto decl : tu.declarations) {
		codegen(state, decl);
	}

	return nullptr;
}

llvm::Value *codegen(codegen::state &state) {
	return codegen(state, *state.translation_unit);
}

} // namespace catalyst::compiler::codegen
