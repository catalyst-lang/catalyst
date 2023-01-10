// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "codegen.hpp"
#include "../../../parser/src/parser.hpp"
#include "decl.hpp"
#include <iostream>

namespace catalyst::compiler::codegen {

void state::report_message_static(report_type type, const std::string &message) {
	parser::report_message(type, message);
}

void state::report_message(report_type type, const std::string &message) {
	if (type == report_type::error) num_errors++;
	if (type == report_type::warning) num_warnings++;
	parser::report_message(type, message);
}

void state::report_message(report_type type, const std::string &message,
                           const parser::ast_node &positional,
                           const std::string &pos_comment) {
	if (type == report_type::error) num_errors++;
	if (type == report_type::warning) num_warnings++;
	if (&positional == nullptr) {
		parser::report_message(type, message);
		return;
	}
	parser::report_message(type, this->translation_unit->parser_state, message, positional,
	                       pos_comment);
}

state::state()
		: TheContext(std::make_unique<llvm::LLVMContext>()), Builder(*TheContext),
		  scopes(&symbol_table), runtime(new compiler::runtime(*this)) {}

void codegen(codegen::state &state, ast::translation_unit &tu) {
	// fill prototypes
	int pass_n = 0;
	int pass_changes = 1;
	while (pass_changes > 0) {
		pass_changes = 0;
		for (const auto &decl : tu.declarations) {
			pass_changes += proto_pass(state, pass_n, decl);
		}

		if (state.num_errors > 0) {
			state.report_message(report_type::info, "Compilation stopped due to errors");
			return;
		}

		pass_n++;
	}

	for (const auto &[k, v] : state.symbol_table) {
		//std::cout << k << ": " << v.type->get_fqn() << std::endl;
		if (v.type == nullptr || !v.type->is_valid()) {
			state.report_message(report_type::error,
			                     "No type has been defined and can't be inferred", *v.ast_node);
		}
	}

	for (const auto &decl : tu.declarations) {
		state.Builder.SetInsertPoint(&state.init_function->getEntryBlock());
		
		codegen(state, decl);
	}
}

void codegen(codegen::state &state) {
	assert(state.init_function == nullptr);
	
	// create init function
	std::vector<llvm::Type *> args(0);
	auto *FT = llvm::FunctionType::get(llvm::Type::getVoidTy(*state.TheContext), false);
	state.init_function = 
		llvm::Function::Create(FT, llvm::Function::ExternalLinkage, ".__CATA_INIT", state.TheModule.get());
	state.init_function->addFnAttr(llvm::Attribute::AlwaysInline);
	state.init_function->setDSOLocal(true);
	auto *BB = llvm::BasicBlock::Create(*state.TheContext, "init", state.init_function);
	state.Builder.SetInsertPoint(BB);

	codegen(state, *state.translation_unit); 

	state.Builder.SetInsertPoint(BB);
	state.Builder.CreateRetVoid();
}

} // namespace catalyst::compiler::codegen
