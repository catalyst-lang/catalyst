// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>

#include "catalyst/rtti.hpp"
#include "decl.hpp"
#include "decl_type.hpp"
#include "expr.hpp"
#include "expr_type.hpp"
#include "stmt.hpp"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
void codegen(codegen::state &state, ast::decl_ptr decl) {
	//	if (std::dynamic_pointer_cast<ast::decl_fn>(decl)) {
	//		return codegen(state, *(ast::decl_fn *)decl.get());
	//	}
	if (isa<ast::decl_fn>(decl)) {
		codegen(state, *(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_var>(decl)) {
		codegen(state, *(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		codegen(state, *(ast::decl_struct *)decl.get());
	} else {
		state.report_message(report_type::error, "Decl type not implemented", decl.get());
	}
}

/// Recursively go over all nested local variables in an AST node and add them to the symbol_table
/// @return number of locals added or changed in this pass
int locals_pass(codegen::state &state, int n, ast::statement_ptr &stmt);

int locals_pass(codegen::state &state, int n,
                std::vector<catalyst::ast::statement_ptr> &statements) {
	int locals_changed = 0;
	for (auto &stmt : statements) {
		locals_changed += locals_pass(state, n, stmt);
	}
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_decl &stmt) {
	return locals_pass(state, n, stmt.decl);
}

int locals_pass(codegen::state &state, int n, ast::decl_var &stmt) {
	auto key = state.scopes.get_fully_qualified_scope_name(stmt.ident.name);
	int locals_changed = 0;

	// get the existing symbol OR emplace a new symbol
	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &stmt, nullptr, type::create_builtin());
	auto &sym = res->second;

	// if first pass and symbol is NOT introduced, this is a redefinition error
	if (n == 0 && !symbol_introduced) {
		state.report_message(report_type::error, "Symbol redefined", &stmt.ident);
		return 0;
		// todo: show where first definition is
		// state.report_message("First definition here", sym.ast_node);
	}

	if (symbol_introduced)
		locals_changed = 1;

	std::shared_ptr<type> new_type;

	if (stmt.type.has_value()) {
		new_type = type::create(state, stmt.type.value());
	} else if (stmt.expr.has_value()) {
		new_type = expr_resulting_type(state, stmt.expr.value());
	} else {
		// TODO: search for first assignment and infer type
		return 0;
	}

	if (*sym.type != *new_type) {
		sym.type = new_type;
		locals_changed = 1;
	}
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_if &stmt) {
	int locals_changed = locals_pass(state, n, stmt.then);
	if (stmt.else_.has_value())
		locals_changed += locals_pass(state, n, stmt.else_.value());
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_block &stmt) {
	std::stringstream sstream;
	sstream << std::hex << (size_t)(&stmt);
	state.scopes.enter(sstream.str());
	int locals_changed = locals_pass(state, n, stmt.statements);
	state.scopes.leave();
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_return &stmt) {
	state.current_function_has_return = true;

	std::shared_ptr<type> expr_type;
	if (stmt.expr.has_value()) {
		expr_type = expr_resulting_type(state, stmt.expr.value());
	} else {
		expr_type = type::create(state, "void");
	}

	type_function *fn_type = (type_function *)state.current_function_symbol->type.get();

	if (!fn_type->return_type->is_valid() && expr_type->is_valid()) {
		fn_type->return_type = expr_type;
		return 1;
	}
	if (fn_type->return_type->is_valid() && expr_type->is_valid() &&
	    *fn_type->return_type != *expr_type) {
		// Mixed return types, codegen will check if this is legal
		return 0;
	} else {
		return 0;
	}
}

int locals_pass(codegen::state &state, int n, ast::statement_ptr &stmt) {
	if (isa<ast::statement_decl>(stmt)) {
		return locals_pass(state, n, *(ast::statement_decl *)stmt.get());
	} else if (isa<ast::statement_if>(stmt)) {
		return locals_pass(state, n, *(ast::statement_if *)stmt.get());
	} else if (isa<ast::statement_block>(stmt)) {
		return locals_pass(state, n, *(ast::statement_block *)stmt.get());
	} else if (isa<ast::statement_return>(stmt)) {
		return locals_pass(state, n, *(ast::statement_return *)stmt.get());
	} else if (isa<ast::statement_expr>(stmt)) {
		// TODO
		return 0;
	}
	state.report_message(report_type::error, "Choices exhausted", stmt.get());
	return 0;
}

/// Perform a locals pass for a function declaration
int locals_pass(codegen::state &state, int n, ast::decl_fn &decl) {
	state.scopes.enter(decl.ident.name);

	int pass_changes = 0;
	if (n == 0) {
		for (auto &param : decl.parameter_list) {
			if (!param.type.has_value()) {
				state.report_message(report_type::error, "Parameter has no type", &param);
				return 0;
			}
			auto key = state.scopes.get_fully_qualified_scope_name(param.ident.name);
			state.symbol_table[key] =
				symbol(&param, nullptr, codegen::type::create(state, param.type.value()));
			pass_changes++;
		}
	}

	if (std::holds_alternative<ast::fn_body_block>(decl.body)) {
		auto &block = std::get<ast::fn_body_block>(decl.body);
		pass_changes = locals_pass(state, n, block.statements);
	}

	state.scopes.leave();
	return pass_changes;
}

int locals_pass(codegen::state &state, int n, ast::decl_ptr &decl) {
	if (isa<ast::decl_var>(decl)) {
		return locals_pass(state, n, *(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_fn>(decl)) {
		return locals_pass(state, n, *(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		state.report_message(report_type::error, "Local structs not supported (yet)", decl.get());
		return 0;
		// return locals_pass(state, n, *(ast::decl_struct *)decl.get());
	}
	state.report_message(report_type::error, "Choices exhausted", decl.get());
	return 0;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(codegen::state &state, llvm::Function *the_function,
                                                const std::string &var_name, type &type) {
	llvm::IRBuilder<> TmpB(&the_function->getEntryBlock(), the_function->getEntryBlock().begin());
	if (isa<type_function>(type)) {
		return TmpB.CreateAlloca(TmpB.getPtrTy(), nullptr, var_name);
	} else if (isa<type_object>(type)) {
		auto *to = (type_object *)&type;
		return TmpB.CreateAlloca(to->object_type->get_llvm_type(state), nullptr, var_name);
	} else if (isa<type_void>(type)) {
		return nullptr;
	} else {
		return TmpB.CreateAlloca(type.get_llvm_type(state), nullptr, var_name);
	}
}

void codegen(codegen::state &state, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto type = (type_function *)sym.type.get();
	auto the_function = (llvm::Function *)sym.value;

	auto previous_function = state.current_function;
	auto previous_function_symbol = state.current_function_symbol;
	state.current_function = the_function;
	state.current_function_symbol = &sym;
	state.scopes.enter(decl.ident.name);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(*state.TheContext, "entry", the_function);
	state.Builder.SetInsertPoint(BB);

	auto previous_return = state.current_return;
	auto previous_return_block = state.current_return_block;
	if (isa<type_void>(type->return_type)) {
		state.current_return = nullptr;
	} else {
		state.current_return =
			CreateEntryBlockAlloca(state, the_function, "ret", *type->return_type);
	}
	state.current_return_block = llvm::BasicBlock::Create(*state.TheContext, "return");

	for (auto &[name, variable] : state.scopes.get_locals()) {
		variable->value = CreateEntryBlockAlloca(state, the_function, name, *variable->type);
	}

	// Record the function arguments in the NamedValues map.
	for (auto &Arg : the_function->args()) {
		// Create an alloca for this variable.
		if (type->is_method() && Arg.getArgNo() == 0) continue;

		auto &arg_local =
			state.symbol_table[state.scopes.get_fully_qualified_scope_name(Arg.getName().str())];

		if (isa<type_object>(arg_local.type)) {
			auto to = (type_object *)arg_local.type.get();
			if (isa<type_struct>(to->object_type)) {
				arg_local.value = &Arg;
			} else {
				state.Builder.CreateStore(&Arg, arg_local.value);
			}
		} else {
			// Store the initial value into the alloca.
			state.Builder.CreateStore(&Arg, arg_local.value);
		}

		// Add arguments to variable symbol table.
		// state.scopes.back().named_values[std::string(Arg.getName())] = arg_local.value;
	}

	if (key == "main") {
		state.Builder.CreateCall(state.init_function);
	}

	codegen(state, decl.body);

	if (!state.Builder.GetInsertBlock()->getTerminator()) {
		state.Builder.CreateBr(state.current_return_block);
	}

	state.current_return_block->insertInto(the_function);
	state.Builder.SetInsertPoint(state.current_return_block);

	if (isa<type_void>(type->return_type)) {
		state.Builder.CreateRetVoid();
	} else {
		state.Builder.CreateRet(state.Builder.CreateLoad(state.current_return->getAllocatedType(),
		                                                 state.current_return));
	}

	// Validate the generated code, checking for consistency.
	std::string err;
	llvm::raw_string_ostream err_output(err);
	if (!llvm::verifyFunction(*the_function, &err_output)) {
		//		printf("Read function definition:\n");
		//		the_function->print(llvm::outs());
		//		printf("\n");

		state.FPM->run(*the_function);
		// the_function->viewCFG();

		state.scopes.pop_back();
	} else {
		// Error reading body, remove function.
		state.report_message(report_type::error, err, &decl);
		the_function->print(llvm::errs());
		state.scopes.pop_back();
		the_function->eraseFromParent();
	}

	state.current_function = previous_function;
	state.current_function_symbol = previous_function_symbol;
	state.current_return = previous_return;
	state.current_return_block = previous_return_block;
}

void codegen(codegen::state &state, ast::fn_body &body) {
	if (std::holds_alternative<ast::fn_body_block>(body)) {
		codegen(state, std::get<ast::fn_body_block>(body));
	} else if (std::holds_alternative<ast::fn_body_expr>(body)) {
		codegen(state, std::get<ast::fn_body_expr>(body));
	} else {
		state.report_message(report_type::error, "unsupported body type");
	}
}

void codegen(codegen::state &state, ast::fn_body_expr &body) {
	auto expr = codegen(state, body.expr);
	state.Builder.CreateRet(expr);
}

void codegen(codegen::state &state, ast::fn_body_block &body) { codegen(state, body.statements); }

int proto_pass(codegen::state &state, int n, ast::decl_ptr decl) {
	if (isa<ast::decl_fn>(decl)) {
		return proto_pass(state, n, *(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_var>(decl)) {
		return proto_pass(state, n, *(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		return proto_pass(state, n, *(ast::decl_struct *)decl.get());
	}
	return 0;
}

int proto_pass(codegen::state &state, int n, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	// llvm::Function *the_function = state.TheModule->getFunction(decl.ident.name);

	symbol* method_of = nullptr;
	if (state.symbol_table.contains(state.scopes.get_fully_qualified_scope_name())) {
		auto parent = state.symbol_table[state.scopes.get_fully_qualified_scope_name()];
		if (isa<type_struct>(parent.type)) method_of = &parent;
	}

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Function name already exists", &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

	auto fn_type = decl_get_type(state, decl);
	if (method_of) {
		((type_function *)fn_type.get())->method_of = std::dynamic_pointer_cast<type_custom>(method_of->type);
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
	int pass_n = n;
	while (locals_updated > 0) {
		locals_updated = locals_pass(state, pass_n++, decl);
		changed_num += locals_updated;
	}

	if (!state.current_function_has_return) {
		if (!decl.type.has_value() || (decl.type.has_value() && decl.type.value().ident.name == "void")) {
			if (!isa<type_void>(current_fn_type->return_type)) {
				current_fn_type->return_type = type::create(state, "void");
				changed_num++;
			}
		} else {
			state.report_message(report_type::error, "control reaches end of non-void function", &decl);
		}
	}

	if (changed_num) {
		// redefine the llvm type
		if (sym.value) {
			((llvm::Function *)sym.value)->eraseFromParent();
		}
		auto the_function = llvm::Function::Create(
			(llvm::FunctionType *)current_fn_type->get_llvm_type(state),
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
				if (isa<type_struct>(to->object_type)) {
					Arg.addAttr(llvm::Attribute::NoUndef);
					Arg.addAttr(llvm::Attribute::getWithByValType(
						*state.TheContext, current_fn_type->parameters[i]->get_llvm_type(state)));
				}
			}
			i++;
		}

		sym.value = the_function;
	}

	state.current_function_symbol = prev_fn_sym;

	return changed_num;
}

int proto_pass(codegen::state &state, int n, ast::decl_var &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Global variable name already exists", &decl.ident);
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

void codegen(codegen::state &state, ast::decl_var &decl) {
	// the locals pass already made sure there is a value in the symbol table
	auto var = state.scopes.find_named_symbol(decl.ident.name);
	if (decl.expr.has_value() && decl.expr.value() != nullptr) {
		codegen_assignment(state, var->value, var->type, decl.expr.value());
	} else {
		// set default value
		auto default_val = var->type->get_default_llvm_value(state);
		if (default_val) {
			state.Builder.CreateStore(default_val, var->value);
		}
	}
}

} // namespace catalyst::compiler::codegen
