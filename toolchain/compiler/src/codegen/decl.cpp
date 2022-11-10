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

void locals_pass(codegen::state &state, ast::statement_ptr &stmt);

void locals_pass(codegen::state &state, std::vector<catalyst::ast::statement_ptr> &statements) {
	for (auto &stmt : statements) {
		locals_pass(state, stmt);
	}
}

void locals_pass(codegen::state &state, ast::statement_var &stmt) {
	state.symbol_table[state.scopes.get_fully_qualified_scope_name(stmt.ident.name)] = {&stmt};
}

void locals_pass(codegen::state &state, ast::statement_if &stmt) {
	locals_pass(state, stmt.then);
	if (stmt.else_.has_value())
		locals_pass(state, stmt.else_.value());
}

void locals_pass(codegen::state &state, ast::statement_block &stmt) {
	std::stringstream sstream;
	sstream << std::hex << (size_t)(&stmt);
	state.scopes.enter(sstream.str());
	locals_pass(state, stmt.statements);
	state.scopes.leave();
}

void locals_pass(codegen::state &state, ast::statement_ptr &stmt) {
	if (typeid(*stmt) == typeid(ast::statement_var)) {
		locals_pass(state, *(ast::statement_var *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_const)) {
		locals_pass(state, *(ast::statement_var *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_if)) {
		locals_pass(state, *(ast::statement_if *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_block)) {
		locals_pass(state, *(ast::statement_block *)stmt.get());
	}
}

void locals_pass(codegen::state &state, ast::decl_fn &decl) {
	state.scopes.enter(decl.ident.name);

	for (auto &param : decl.parameter_list) {
		state.symbol_table[state.scopes.get_fully_qualified_scope_name(param.ident.name)] = {
			&param};
	}

	if (std::holds_alternative<ast::fn_body_block>(decl.body)) {
		auto &block = std::get<ast::fn_body_block>(decl.body);
		locals_pass(state, block.statements);
	}

	state.scopes.leave();
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(codegen::state &state, llvm::Function *TheFunction,
                                                const std::string &VarName) {
	llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
	return TmpB.CreateAlloca(llvm::Type::getInt64Ty(*state.TheContext), 0, VarName.c_str());
}

void codegen(codegen::state &state, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	auto the_function =
		(llvm::Function *)state
			.symbol_table[state.scopes.get_fully_qualified_scope_name(decl.ident.name)]
			.value;

	state.current_function = the_function;
	state.scopes.enter(decl.ident.name);

	llvm::BasicBlock *BB = llvm::BasicBlock::Create(*state.TheContext, "entry", the_function);
	state.Builder.SetInsertPoint(BB);

	state.current_return = CreateEntryBlockAlloca(state, the_function, "ret");
	state.current_return_block = llvm::BasicBlock::Create(*state.TheContext, "return");

	for (auto &[name, variable] : state.scopes.get_locals()) {
		variable->value = CreateEntryBlockAlloca(state, the_function, name);
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
	state.Builder.CreateRet(codegen(state, body.expr));
}

void codegen(codegen::state &state, ast::fn_body_block &body) {
	for (auto &stmt : body.statements) {
		codegen(state, stmt);
	}
}

void proto_pass(codegen::state &state, ast::decl_ptr decl) {
	if (typeid(*decl) == typeid(ast::decl_fn)) {
		proto_pass(state, *(ast::decl_fn *)decl.get());
	}
}

void proto_pass(codegen::state &state, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	// llvm::Function *the_function = state.TheModule->getFunction(decl.ident.name);

	if (state.symbol_table.count(state.scopes.get_fully_qualified_scope_name(decl.ident.name)) >
	    0) {
		state.report_error("Function name already exists", decl.ident);
		return;
	}

	// Make the function type:  double(double,double) etc.
	std::vector<llvm::Type *> ints(decl.parameter_list.size(),
	                               llvm::Type::getInt64Ty(*state.TheContext));
	llvm::FunctionType *FT =
		llvm::FunctionType::get(llvm::Type::getInt64Ty(*state.TheContext), ints, false);

	auto the_function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, decl.ident.name,
	                                           state.TheModule.get());

	// Set names for all arguments.
	unsigned Idx = 0;
	for (auto &Arg : the_function->args())
		Arg.setName(decl.parameter_list[Idx++].ident.name);

	state.symbol_table[state.scopes.get_fully_qualified_scope_name(decl.ident.name)] = {
		&decl, the_function};

	locals_pass(state, decl);
}

} // namespace catalyst::compiler::codegen
