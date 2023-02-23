#pragma once
#include "../common/catalyst/ast/ast.hpp"
#include "../compiler.hpp"
#include "type.hpp"
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
#include <ranges>
#include <vector>

#include "symbol.hpp"
#include "catalyst/rtti.hpp"

namespace catalyst::compiler::codegen {

// TODO: move to string helper
inline std::vector<std::string> split(const std::string &s, const std::string &delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}


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
		root_scope = &emplace_back(nullptr, "");
	}

	// TODO: find_named_symbol and find_overloaded_symbol only find up the scope tree or full names,
	// but do not find "d.e" if the scope tree is 'a.b.c.d.f'.

	symbol *find_named_symbol(const std::string &name, bool exact_match = false) {
		if (!exact_match)
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

	symbol *find_named_symbol(const ast::type_qualified_name &name, bool exact_match = false) {
		return find_named_symbol(name.to_string(), exact_match);
	}

	std::set<symbol *> find_overloaded_symbol(const std::string &name, bool exact_match = false, bool look_in_multiple_scopes = false) {
		std::set<symbol *> results;

		if (!exact_match)
			for (auto it = rbegin(); it != rend(); ++it) {
				int i = 1;
				auto potential_local_name = (*it).get_fully_qualified_scope_name(name);
				auto key = potential_local_name;
				while (symbol_table->count(key) > 0) {
					results.insert(&(*symbol_table)[key]);
					key = potential_local_name + "`" + std::to_string(i++);
				}
				if (!look_in_multiple_scopes && results.size() > 0) break;
			}
		if (symbol_table->count(name) > 0) {
			int i = 1;
			auto key = name;
			while (symbol_table->count(key) > 0) {
				results.insert(&(*symbol_table)[key]);
				key = name + "`" + std::to_string(i++);
			}
		}

		return results;
	}

	inline void enter(const std::string &name) { emplace_back(&back(), name); }
	inline void enter_fqn(const std::string &fqn) {
		while (size() > 1) pop_back(); // clear to root

		for (const auto& ident : split(fqn, ".")) {
			enter(ident);
		}
	}
	inline void enter_ns(std::shared_ptr<type_ns> ns) {
		auto q = std::find_if(
			symbol_table->begin(), symbol_table->end(),
			[&](const std::pair<std::string, symbol> &pair) { return pair.second.type == ns; });
		if (q != symbol_table->end()) {
			std::string key = (*q).first;
			enter_fqn(key);
		} else {
			std::cout << "SHOULD NOT HAPPEN @ scope::enter_ns" << std::endl;
		}
	}
	inline void leave() { pop_back(); }

	inline scope &current_scope() { return back(); }

	inline bool is_root_scope() { return &current_scope() == root_scope; }
	inline bool is_ns_scope() {
		auto key = current_scope().get_fully_qualified_scope_name();
		if (!symbol_table->contains(key)) return false;
		return isa<type_ns>((*symbol_table)[key].type);
	}
	inline bool is_root_or_ns_scope() { return is_root_scope() || is_ns_scope(); }

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

} // namespace catalyst::compiler::codegen
