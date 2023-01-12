#pragma once
#include "../common/catalyst/ast/ast.hpp"
#include "../compiler.hpp"
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

#include "symbol.hpp"

namespace catalyst::compiler::codegen {

struct scope {
	std::string name;
	scope *parent;

	scope() = delete;
	scope(scope *parent, const std::string &name) : parent(parent), name(name) {}

	inline std::string get_fully_qualified_scope_name(const std::string &append = std::string()) {
		std::vector<std::string> scope_names;
		scope_names.push_back(name);
		scope *scope = this;
		while (scope->parent != nullptr) {
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
		if (symbol_table->count(name) > 0) {
			return &(*symbol_table)[name];
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

}
