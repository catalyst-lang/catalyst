// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "codegen.hpp"
#include "../runtime.hpp"
#include "../../../parser/src/parser.hpp"
#include "decl.hpp"
#include "catalyst/rtti.hpp"
#include "decl_proto_pass.hpp"
#include "decl_overloading_pass.hpp"
#include <iostream>

namespace catalyst::compiler::codegen {

void state::report_message(report_type type, const std::string &message, std::ostream& os) {
	if (type == report_type::error) num_errors++;
	if (type == report_type::warning) num_warnings++;
	parser::report_message(type, message, os);
}

void state::report_message(report_type type, const std::string &message,
                           const parser::ast_node *positional,
                           const std::string &pos_comment, std::ostream& os) {
	if (type == report_type::error) num_errors++;
	if (type == report_type::warning) num_warnings++;
	if (positional == nullptr) {
		parser::report_message(type, message, os);
		return;
	}
	parser::report_message(type, this->translation_unit->parser_state, message, *positional,
	                       pos_comment, os);
}

state::state()
		: TheContext(std::make_unique<llvm::LLVMContext>()), Builder(*TheContext),
		  scopes(&symbol_table), target(new compiler::runtime(*this)) {}

#define STOP_IF_ERROR() {\
	if (state.num_errors > 0) {\
		state.report_message(report_type::info, "Compilation stopped due to errors");\
		return;\
	}\
}0

void codegen(codegen::state &state, ast::translation_unit &tu) {
	// check for global namespace decl
	int num_global_ns = 0;
	for (int i = 0; i < tu.declarations.size(); i++) {
		if (isa<ast::decl_ns>(tu.declarations[i])) {
			auto ns = std::static_pointer_cast<ast::decl_ns>(tu.declarations[i]);
			if (ns->is_global) {
				if (num_global_ns > 0) {
					state.report_message(report_type::error, "Multiple global namespace definitions", ns.get());
				} else {
					// do namespace stuff
					if (i != 0) {
						state.report_message(report_type::warning, "Global namespace specifier not the first declaration of the file", ns.get());
						state.report_message(report_type::help, "Move the namespace specifier to the top of the file to prevent confusion");
					}
					state.scopes.enter(ns->ident.name);
					state.global_namespace = ns->ident.name;
				}
				num_global_ns++;
			}
		}
	}

	overloading_pass op(state);
	op(tu);
	STOP_IF_ERROR();

	// fill prototypes
	proto_pass proto_pass(state);
	int pass_changes = 1;
	while (pass_changes > 0) {
		pass_changes = 0;
		pass_changes += proto_pass(tu);
		STOP_IF_ERROR();
	}

	#ifndef NDEBUG
	for (const auto &[k, v] : state.symbol_table) {
		std::cout << k << ": " << v.type->get_fqn() << std::endl;
	}
	#endif

	for (const auto &[k, v] : state.symbol_table) {
		if (v.type == nullptr || !v.type->is_valid()) {
			state.report_message(report_type::error,
			                     "No type has been defined and can't be inferred", v.ast_node);					 
			return;
		}
	}

	for (const auto &decl : tu.declarations) {
		state.Builder.SetInsertPoint(&state.init_function->getEntryBlock());
		
		codegen(state, decl);
	}

	if (num_global_ns > 0) {
		state.scopes.leave();
	}
}

void codegen(codegen::state &state) {
	assert(state.init_function == nullptr);
	
	// create init function
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
	if (!state.TheFPM->isEmpty()) {
		state.TheFPM->run(*state.init_function, *state.TheFAM);
	}
}

} // namespace catalyst::compiler::codegen
