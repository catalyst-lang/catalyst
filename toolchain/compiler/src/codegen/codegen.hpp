// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#pragma warning(disable : 4624)

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <map>
#include <memory>
#include <deque>

#include "../compiler.hpp"
#include "../common/catalyst/ast/ast.hpp"

namespace catalyst::compiler::codegen {

struct scope {
	std::string name;
	std::map<std::string, llvm::Value *> named_values;
	llvm::Function *current_function = nullptr;

	scope() = delete;
	scope(const std::string &name) : name(name) {
	}
};

struct scope_stack : public std::deque<scope> {
	scope *root_scope;

	scope_stack() {
		root_scope = &emplace_back("root");
	}

	llvm::Value *get_named_value(const std::string &name) {
		for (auto it = rbegin(); it != rend(); ++it) {
			if ((*it).named_values.contains(name)) {
				return (*it).named_values[name];
			}
		}
		return nullptr;
	}

	inline scope& current_scope() {
		return back();
	}

	inline bool is_root_scope() {
		return &current_scope() == root_scope;
	}

};

struct state {
	compiler::options options;
	std::unique_ptr<llvm::LLVMContext> TheContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> TheModule;
	std::unique_ptr<llvm::legacy::FunctionPassManager> FPM;

	bool is_success = true;

	catalyst::ast::translation_unit *translation_unit{};

	scope_stack scopes;

	state() : TheContext(std::make_unique<llvm::LLVMContext>()), Builder(*TheContext) {
	}

	static void report_error(const std::string &error);
	void report_error(const std::string &error, const parser::positional &positional,
	                  const std::string &pos_comment = "here") const;

	scope &current_scope() {
		return scopes.current_scope();
	}

	scope &root_scope() {
		return *scopes.root_scope;
	}

	bool is_root_scope() {
		return scopes.is_root_scope();
	}
};

llvm::Value *codegen(codegen::state &state, ast::translation_unit &tu);
llvm::Value *codegen(codegen::state &state);

} // namespace catalyst::compiler::codegen

