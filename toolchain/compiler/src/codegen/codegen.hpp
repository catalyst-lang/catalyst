// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#pragma warning(disable : 4624)

namespace catalyst::compiler::codegen {
struct state;
}

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <deque>
#include <map>
#include <memory>

#include "../../../parser/src/parser.hpp"
#include "../common/catalyst/ast/ast.hpp"
#include "../common/catalyst/ast/parser.hpp"
#include "../compiler.hpp"
#include "scope.hpp"
#include "symbol.hpp"

namespace catalyst::compiler::codegen {

using report_type = parser::report_type;

struct state {
	compiler::options options;
	std::unique_ptr<llvm::LLVMContext> TheContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> TheModule;
	std::unique_ptr<llvm::legacy::FunctionPassManager> FPM;

	symbol *current_function_symbol = nullptr;
	llvm::Function *current_function = nullptr;
	llvm::AllocaInst *current_return = nullptr;
	llvm::BasicBlock *current_return_block = nullptr;

	symbol_map symbol_table;

	catalyst::ast::translation_unit *translation_unit{};

	scope_stack scopes;

	state()
		: TheContext(std::make_unique<llvm::LLVMContext>()), Builder(*TheContext),
		  scopes(&symbol_table) {}

	static void report_message(report_type type, const std::string &message);
	void report_message(report_type type, const std::string &message,
	                    const parser::ast_node &positional,
	                    const std::string &pos_comment = "here") const;

	scope &current_scope() { return scopes.current_scope(); }

	scope &root_scope() { return *scopes.root_scope; }

	bool is_root_scope() { return scopes.is_root_scope(); }
};

llvm::Value *codegen(codegen::state &state, ast::translation_unit &tu);
llvm::Value *codegen(codegen::state &state);

} // namespace catalyst::compiler::codegen
