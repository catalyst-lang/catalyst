// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>

#include "decl.hpp"
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
	if (typeid(*decl) == typeid(ast::decl_fn)) {
		codegen(state, *(ast::decl_fn *)decl.get());
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

int locals_pass(codegen::state &state, int n, ast::statement_var &stmt) {
	auto key = state.scopes.get_fully_qualified_scope_name(stmt.ident.name);
	int locals_changed = 0;

	// get the existing symbol OR emplace a new symbol
	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &stmt, nullptr, type::create(""));
	auto &sym = res->second;

	// if first pass and symbol is NOT introduced, this is a redefinition error
	if (n == 0 && !symbol_introduced) {
		state.report_error("Symbol redefined", stmt.ident);
		return 0;
		// todo: show where first definition is
		// state.report_error("First definition here", sym.ast_node);
	}

	if (symbol_introduced)
		locals_changed = 1;

	std::shared_ptr<type> new_type;

	if (stmt.type.has_value()) {
		new_type = type::create(stmt.type.value());
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
	std::shared_ptr<type> expr_type = expr_resulting_type(state, stmt.expr);

	type_function *fn_type = (type_function *)state.current_function_symbol->type.get();

	if (!fn_type->return_type->is_valid && expr_type->is_valid) {
		fn_type->return_type = expr_type;
		return 1;
	}
	if (fn_type->return_type->is_valid && expr_type->is_valid &&
	    *fn_type->return_type != *expr_type) {
		// Mixed return types, codegen will check if this is legal
		return 0;
	} else {
		return 0;
	}
}

int locals_pass(codegen::state &state, int n, ast::statement_ptr &stmt) {
	if (typeid(*stmt) == typeid(ast::statement_var)) {
		return locals_pass(state, n, *(ast::statement_var *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_const)) {
		return locals_pass(state, n, *(ast::statement_var *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_if)) {
		return locals_pass(state, n, *(ast::statement_if *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_block)) {
		return locals_pass(state, n, *(ast::statement_block *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_return)) {
		return locals_pass(state, n, *(ast::statement_return *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_expr)) {
		// TODO
		return 0;
	}
	state.report_error("Choices exhausted");
	return 0;
}

/// Perform a locals pass for a function declaration
int locals_pass(codegen::state &state, int n, ast::decl_fn &decl) {
	state.scopes.enter(decl.ident.name);

	int pass_changes = 0;
	if (n == 0) {
		for (auto &param : decl.parameter_list) {
			if (!param.type.has_value()) {
				state.report_error("Parameter has no type", param);
				return 0;
			}
			auto key = state.scopes.get_fully_qualified_scope_name(param.ident.name);
			state.symbol_table[key] =
				symbol(&param, nullptr, codegen::type::create(param.type.value()));
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

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(codegen::state &state, llvm::Function *TheFunction,
                                                const std::string &VarName, const type &type) {
	llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
	return TmpB.CreateAlloca(type.get_llvm_type(state), 0, VarName.c_str());
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
	state.current_return = CreateEntryBlockAlloca(state, the_function, "ret", *type->return_type);
	state.current_return_block = llvm::BasicBlock::Create(*state.TheContext, "return");

	for (auto &[name, variable] : state.scopes.get_locals()) {
		variable->value = CreateEntryBlockAlloca(state, the_function, name, *variable->type);
	}

	// Record the function arguments in the NamedValues map.
	for (auto &Arg : the_function->args()) {
		// Create an alloca for this variable.
		auto &arg_local =
			state.symbol_table[state.scopes.get_fully_qualified_scope_name(Arg.getName().str())];

		// Store the initial value into the alloca.
		state.Builder.CreateStore(&Arg, arg_local.value);

		// Add arguments to variable symbol table.
		// state.scopes.back().named_values[std::string(Arg.getName())] = arg_local.value;
	}

	codegen(state, decl.body);

	state.Builder.CreateBr(state.current_return_block);

	state.current_return_block->insertInto(the_function);
	state.Builder.SetInsertPoint(state.current_return_block);
	state.Builder.CreateRet(
		state.Builder.CreateLoad(state.current_return->getAllocatedType(), state.current_return));

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
		state.report_error(err, decl);
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
		state.report_error("unsupported body type");
	}
}

void codegen(codegen::state &state, ast::fn_body_expr &body) {
	auto expr = codegen(state, body.expr);
	state.Builder.CreateRet(expr);
}

void codegen(codegen::state &state, ast::fn_body_block &body) {
	for (auto &stmt : body.statements) {
		codegen(state, stmt);
	}
}

int proto_pass(codegen::state &state, int n, ast::decl_ptr decl) {
	if (typeid(*decl) == typeid(ast::decl_fn)) {
		return proto_pass(state, n, *(ast::decl_fn *)decl.get());
	}
	return 0;
}

int proto_pass(codegen::state &state, int n, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	// llvm::Function *the_function = state.TheModule->getFunction(decl.ident.name);

	if (n == 0 &&
	    state.symbol_table.contains(state.scopes.get_fully_qualified_scope_name(decl.ident.name))) {
		state.report_error("Function name already exists", decl.ident);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

	auto return_type = decl.type.has_value() ? type::create(decl.type.value()) : type::create();

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &decl, nullptr, type::create_function(return_type));
	auto &sym = res->second;

	symbol *prev_fn_sym = state.current_function_symbol;
	state.current_function_symbol = &sym;

	// Make the function type
	std::vector<std::shared_ptr<type>> params;
	for (const auto &param : decl.parameter_list) {
		if (!param.type.has_value()) {
			state.report_error("Parameter has no type", param);
			return 0;
		}
		params.push_back(type::create(param.type.value()));
	}

	auto current_fn_type = (type_function *)sym.type.get();
	auto fn_type = type::create_function(current_fn_type->return_type, params);

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

	if (changed_num) {
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
	}

	state.current_function_symbol = prev_fn_sym;

	return changed_num;
}

} // namespace catalyst::compiler::codegen
