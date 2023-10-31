// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#pragma warning(disable : 4624)

#include <deque>
#include <map>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "../../../parser/src/parser.hpp"
#include "../common/catalyst/ast/ast.hpp"
#include "../common/catalyst/ast/parser.hpp"
#include "../compiler.hpp"
#include "scope.hpp"
#include "symbol.hpp"

namespace catalyst::compiler {
	struct target;
}

namespace catalyst::compiler::codegen {

using report_type = parser::report_type;

struct state {
	compiler::options options;
	std::unique_ptr<llvm::LLVMContext> TheContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> TheModule;
	std::unique_ptr<llvm::FunctionPassManager> TheFPM;
	std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
	std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
	std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
	std::unique_ptr<llvm::StandardInstrumentations> TheSI;

	symbol *current_function_symbol = nullptr;
	llvm::Function *current_function = nullptr;
	llvm::AllocaInst *current_return = nullptr;
	llvm::BasicBlock *current_return_block = nullptr;
	bool current_function_has_return = false; // TODO: move this to the locals pass

	int num_errors = 0;
	int num_warnings = 0;

	std::string global_namespace = "";

	llvm::Function *init_function = nullptr;

	symbol_map symbol_table;

	catalyst::ast::translation_unit *translation_unit{};

	scope_stack scopes;

	target *target;

	state();

	void report_message(report_type type, const std::string &message, std::ostream& os = std::cout);
	void report_message(report_type type, const std::string &message,
	                    const parser::ast_node *positional,
	                    const std::string &pos_comment = "here", std::ostream& os = std::cout);

	scope &current_scope() { return scopes.current_scope(); }

	scope &root_scope() { return *scopes.root_scope; }

	bool is_root_scope() { return scopes.is_root_scope(); }

	bool is_root_or_ns_scope() { return scopes.is_root_or_ns_scope(); }
};

void codegen(codegen::state &state, ast::translation_unit &tu);
void codegen(codegen::state &state);

} // namespace catalyst::compiler::codegen
