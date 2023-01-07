// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>

#include "decl.hpp"
#include "decl_type.hpp"
#include "expr.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

int proto_pass(codegen::state &state, int n, ast::decl_struct &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	if (n == 0 &&
	    state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Struct name already exists", decl.ident);
		state.report_message(report_type::info, "Previous declaration here", *other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

    auto struct_type = decl_get_type(state, decl);
	((type_struct*)struct_type.get())->name = key;
	const auto [res, symbol_introduced] = state.symbol_table.try_emplace(key, &decl, nullptr, struct_type);
	auto &sym = res->second;

	if (*sym.type != *struct_type) {
		// We can't just reassign sym.type to struct_type, as there might be references to the
		// structure pointed to by sym.type at this point.
		// We only want one instance of type_struct to ever exist per definition.
		((type_struct*)sym.type.get())->copy_from(*(type_struct*)struct_type.get());
		changed_num = 1;
	}

    // TODO: struct functions

	// int locals_updated = 1;
	// int pass_n = n;
	// while (locals_updated > 0) {
	// 	locals_updated = locals_pass(state, pass_n++, decl);
	// 	changed_num += locals_updated;
	// }

	/*if (changed_num) {
		// redefine the llvm type
		if (sym.value) {
			((llvm::Function*)sym.value)->eraseFromParent();
		}
		auto the_function = llvm::Function::Create(
			(llvm::FunctionType *)current_fn_type->get_llvm_type(state),
			llvm::Function::ExternalLinkage, decl.ident.name, state.TheModule.get());

		// Set names for all arguments.
		unsigned Idx = 0;
		for (auto &Arg : the_function->args())
			Arg.setName(decl.parameter_list[Idx++].ident.name);

		sym.value = the_function;
	}*/

	return changed_num;
}

void codegen(codegen::state &state, ast::decl_struct &decl) {
	/*auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto type = (type_struct *)sym.type.get();

    auto llvm_type = type->get_llvm_type(state);

    auto structAlloca = state.Builder.CreateAlloca(llvm_type);*/

}


} // namespace catalyst::compiler::codegen
