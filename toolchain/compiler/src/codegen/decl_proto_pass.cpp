// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "decl_proto_pass.hpp"
#include "decl_locals_pass.hpp"

#include "decl_locals_pass.hpp"

#include "catalyst/rtti.hpp"
#include "decl_type.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

int proto_pass::process(ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	// llvm::Function *the_function = state.TheModule->getFunction(decl.ident.name);

	symbol *method_of = nullptr;
	if (state.symbol_table.contains(state.scopes.get_fully_qualified_scope_name())) {
		auto &parent = state.symbol_table[state.scopes.get_fully_qualified_scope_name()];
		if (isa<type_custom>(parent.type))
			method_of = &parent;
	}

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	// TODO: fix this. Is broken after function overloading was added. No error is reported if
	// function with same name and type is added.
	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Function name already exists", &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

	auto fn_type = decl_get_type(state, decl);
	if (method_of) {
		auto fn = (type_function *)fn_type.get();
		fn->method_of = object_type_reference<type_custom>(state, std::dynamic_pointer_cast<type_custom>(method_of->type));
	}

	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &decl, nullptr, fn_type);
	auto &sym = res->second;

	symbol *prev_fn_sym = state.current_function_symbol;
	state.current_function_symbol = &sym;
	state.current_function_has_return = false;

	auto current_fn_type = (type_function *)sym.type.get();
	if (!((type_function *)fn_type.get())->return_type->is_valid())
		((type_function *)fn_type.get())->return_type = current_fn_type->return_type;
	if (*current_fn_type != *fn_type) {
		sym.type = fn_type;
		current_fn_type = (type_function *)fn_type.get();
		changed_num = 1;
	}

	int locals_updated = 1;
	locals_pass lp(state, n);
	while (locals_updated > 0) {
		locals_updated = lp(&decl);
		if (state.num_errors > 0) return 0;
		changed_num += locals_updated;
	}

	if (!state.current_function_has_return) {
		bool is_return_void = !decl.type.has_value();
		if (decl.type.has_value() && isa<ast::type_qualified_name>(decl.type.value())) {
			auto tqn = std::static_pointer_cast<ast::type_qualified_name>(decl.type.value());
			if (tqn->to_string() == "void") is_return_void = true;
		}
		if (is_return_void) {
			if (!isa<type_void>(current_fn_type->return_type)) {
				current_fn_type->return_type = type::create(state, "void");
				changed_num++;
			}
		} else {
			state.report_message(report_type::error, "control reaches end of non-void function",
			                     &decl);
		}
	}

	if (decl.ident.name == "new") {
		if (!isa<type_void>(current_fn_type->return_type)) {
			state.report_message(report_type::error, "`new` function must return void", &decl);
			return 0;
		}
	}

	if (decl.ident.name == "discard") {
		if (!isa<type_void>(current_fn_type->return_type)) {
			state.report_message(report_type::error, "`discard` function must return void", &decl);
			return 0;
		}
	}

	if (changed_num) {
		// redefine the llvm type
		if (sym.value) {
			((llvm::Function *)sym.value)->eraseFromParent();
		}
		if (current_fn_type->is_valid()) {
			auto the_function =
				llvm::Function::Create((llvm::FunctionType *)current_fn_type->get_llvm_type(state),
									llvm::Function::ExternalLinkage, key, state.TheModule.get());

			// Set names for all arguments.
			unsigned i = method_of != nullptr ? -1 : 0;
			for (auto &Arg : the_function->args()) {
				if (i == -1) {
					// method; this param
					Arg.setName("this");
					i++;
					continue;
				}
				Arg.setName(decl.parameter_list[i].ident.name);
				if (isa<type_object>(current_fn_type->parameters[i])) {
					auto to = (type_object *)current_fn_type->parameters[i].get();
					if (isa<type_struct>(to->object_type.get())) {
						Arg.addAttr(llvm::Attribute::NoUndef);
						Arg.addAttr(llvm::Attribute::getWithByValType(
							*state.TheContext, current_fn_type->parameters[i]->get_llvm_type(state)));
					} else if (isa<type_class>(to->object_type.get())) {
						Arg.addAttr(llvm::Attribute::NoUndef);
					}
				}
				i++;
			}

			sym.value = the_function;
		}
	}

	state.current_function_symbol = prev_fn_sym;

	return changed_num;
}

int proto_pass::process(ast::decl_var &decl) {
	if (!state.is_root_or_ns_scope())
		return 0;

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Global variable name already exists",
		                     &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	auto type = decl_get_type(state, decl);

	const auto [res, symbol_introduced] = state.symbol_table.try_emplace(key, &decl, nullptr, type);
	auto &sym = res->second;

	if (type->is_valid() && sym.value == nullptr) {
		llvm::IRBuilder<> TmpB(&state.init_function->getEntryBlock(),
		                       state.init_function->getEntryBlock().begin());
		state.TheModule->getOrInsertGlobal(key, type->get_llvm_type(state));
		llvm::GlobalVariable *gVar = state.TheModule->getNamedGlobal(key);
		gVar->setDSOLocal(true);
		// gVar->setAlignment(llvm::MaybeAlign(4));
		gVar->setInitializer(type->get_default_llvm_value(state));
		// gVar->setLinkage(llvm::GlobalValue::InternalLinkage);
		sym.value = gVar; // TmpB.CreateAlloca(type->get_llvm_type(state), 0, key.c_str());
	}

	if (n == 0)
		return 1;
	if (*sym.type != *type)
		return 1;
	return 0;
}

int proto_pass::process(ast::decl_ns &decl) {
	std::string key;
	if (decl.is_global) {
		key = decl.ident.name;
	} else {
		key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	}
	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Namespace name already exists",
		                     &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	auto type = std::make_shared<type_ns>(decl.ident.name);
	const auto [res, symbol_introduced] = state.symbol_table.try_emplace(key, &decl, nullptr, type);
	auto &sym = res->second;

	return symbol_introduced ? 1 : 0;
}

} // namespace catalyst::compiler::codegen
