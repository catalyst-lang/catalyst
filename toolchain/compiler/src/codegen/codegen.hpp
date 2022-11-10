// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#pragma warning(disable : 4624)

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

#include "../common/catalyst/ast/ast.hpp"
#include "../compiler.hpp"

namespace catalyst::compiler::codegen {

struct symbol {
	std::variant<ast::statement_var *, ast::fn_parameter *, ast::decl_fn *> ast_node;
	llvm::Value *value;
};

using symbol_map = std::map<std::string, symbol>;
using symbol_map_view = std::map<std::string, symbol *>;

struct scope {
	std::string name;
	scope *parent;

	scope() = delete;
	scope(scope* parent, const std::string &name) : parent(parent), name(name) {}

	inline std::string get_fully_qualified_scope_name(const std::string &append = std::string()) {
		std::vector<std::string> scope_names;
		scope_names.push_back(name);
		scope *scope = this;
		while(scope->parent != nullptr) {
			scope = scope->parent;
			scope_names.push_back(scope->name);
		}

		std::stringstream fqident;
		for (auto it = scope_names.rbegin(); it != scope_names.rend(); it++) {
			if (!fqident.str().empty())
				fqident << ".";
			fqident << *it;
		}
		if (!append.empty()) {
			if (!fqident.str().empty())
				fqident << ".";
			fqident << append;
		}
		return fqident.str();
	}
};

struct scope_stack : public std::deque<scope> {
	scope *root_scope;

  private:
	symbol_map *symbol_table;

  public:
	scope_stack(symbol_map *symbol_table) : symbol_table(symbol_table) {
		root_scope = &emplace_back(nullptr, "root");
	}

	symbol *find_named_symbol(const std::string &name) {
		for (auto it = rbegin(); it != rend(); ++it) {
			auto potential_local_name = (*it).get_fully_qualified_scope_name(name);
			if (symbol_table->count(potential_local_name) > 0) {
				return &(*symbol_table)[potential_local_name];
			}
		}
		return nullptr;
	}

	inline void enter(const std::string &name) { emplace_back(&back(), name); }
	inline void leave() { pop_back(); }

	inline scope &current_scope() { return back(); }

	inline bool is_root_scope() { return &current_scope() == root_scope; }

	inline std::string get_fully_qualified_scope_name(const std::string &append = std::string()) {
		return current_scope().get_fully_qualified_scope_name(append);
	}

	symbol_map_view get_locals() {
		symbol_map_view locals;
		auto fqsn = get_fully_qualified_scope_name() + ".";
		for (auto &[name, variable] : *symbol_table) {
			if (name.starts_with(fqsn))
				locals[name] = &variable;
		}
		return locals;
	}
};

struct state {
	compiler::options options;
	std::unique_ptr<llvm::LLVMContext> TheContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> TheModule;
	std::unique_ptr<llvm::legacy::FunctionPassManager> FPM;

	llvm::Function *current_function = nullptr;
	llvm::AllocaInst *current_return = nullptr;
	llvm::BasicBlock * current_return_block = nullptr;

	symbol_map symbol_table;

	catalyst::ast::translation_unit *translation_unit{};

	scope_stack scopes;

	state()
		: TheContext(std::make_unique<llvm::LLVMContext>()), Builder(*TheContext),
		  scopes(&symbol_table) {}

	static void report_error(const std::string &error);
	void report_error(const std::string &error, const parser::positional &positional,
	                  const std::string &pos_comment = "here") const;

	scope &current_scope() { return scopes.current_scope(); }

	scope &root_scope() { return *scopes.root_scope; }

	bool is_root_scope() { return scopes.is_root_scope(); }
};

llvm::Value *codegen(codegen::state &state, ast::translation_unit &tu);
llvm::Value *codegen(codegen::state &state);

} // namespace catalyst::compiler::codegen
