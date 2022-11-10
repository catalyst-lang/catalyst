// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

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
	for(auto decl : tu.declarations) {
		proto_pass(state, decl);
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
