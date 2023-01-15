// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "function_overloading.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

symbol* find_function_overload(codegen::state &state, const std::string &name, ast::expr_call &expr, std::shared_ptr<type> expecting_type) {
	auto symbols = state.scopes.find_overloaded_symbol(name);
	if (symbols.empty()) return nullptr;
	if (symbols.size() == 1) return *symbols.begin();

	// eliminate the symbols that don't match parameter types and return type
	
	// remove all calls that have a different number of parameters than the call or have incompatible
	// types.
	std::erase_if(symbols, [&](symbol* symbol) { 
			auto type = (type_function*)symbol->type.get();
			if (type->parameters.size() != expr.parameters.size()) return true;
			for (int i = 0; i < type->parameters.size(); i++) {
				auto expr_type = expr_resulting_type(state, expr.parameters[i], type->parameters[i]);
				if (!type->parameters[i]->is_assignable_from(expr_type)) return true;
			}
			return false;
		});

	if (symbols.size() > 1) {
		// we have more than 1 candidate, let's see if there is one with exact type matches
		auto exact_symbols = std::set<symbol *>(symbols);
		std::erase_if(exact_symbols, [&](symbol* symbol) { 
				auto type = (type_function*)symbol->type.get();
				for (int i = 0; i < type->parameters.size(); i++) {
					auto expr_type = expr_resulting_type(state, expr.parameters[i], type->parameters[i]);
					if (*type->parameters[i] != *expr_type) return true;
				}
				return false;
			});
		if (exact_symbols.size() == 1) {
			// we've found exactly 1 exact match. Winner!
			return *exact_symbols.begin();
		}

		// if we still have more than 1 candidate, we can match against return type
		if (!expecting_type || !expecting_type->is_valid()) {
			state.report_message(report_type::error, "function call couldn't be matched by return type", &expr);
			state.report_message(report_type::info, "Expected type is not deductable from context.");
            state.report_message(report_type::help, "Consider making types explicit.");
		} else {
			std::erase_if(symbols, [&](symbol* symbol) { 
					auto type = (type_function*)symbol->type.get();
					if (!expecting_type->is_assignable_from(type->return_type)) return true;
					return false;
				});
		}

		// TODO: maybe again check for exact types in parameters here

        // TODO: again check for for exact return type here
	}

	if (symbols.empty()) {
		state.report_message(report_type::error, "No overload matches call signature", &expr);
		return nullptr;
	}
	if (symbols.size() > 1) {
		state.report_message(report_type::error, "Ambiguous function call", &expr);
		for (auto symbol : symbols) {
			state.report_message(report_type::info, "Possible candidate", symbol->ast_node);
		}
        state.report_message(report_type::help, "Consider making types explicit.");
		return nullptr;
	}

	return *symbols.begin();
}

} // namespace catalyst::compiler::codegen
