// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <memory>
#include <sstream>
#include <typeinfo>

#include "decl.hpp"
#include "expr.hpp"
#include "stmt.hpp"

namespace catalyst::compiler::codegen {

// I know a double dispatch pattern like visitors are generally better to use,
// but the way that would work in C++ is so ugly, it introduces more overhead
// and bloat to the codebase than just this one ugly dispatch function.
// Feel free to refactor the AST and introduce an _elegant_ visitation pattern.
llvm::Value *codegen(codegen::state &state, ast::decl_ptr decl) {
//	if (std::dynamic_pointer_cast<ast::decl_fn>(decl)) {
//		return codegen(state, *(ast::decl_fn *)decl.get());
//	}
	if (typeid(*decl) == typeid(ast::decl_fn)) {
		return codegen(state, *(ast::decl_fn *)decl.get());
	}

	return nullptr;
}

llvm::Value *codegen(codegen::state &state, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	auto the_function = (llvm::Function *)state.current_scope().named_values[decl.ident.name];

	// Record the function arguments in the NamedValues map.
	state.scopes.emplace_back(decl.ident.name);
	for (auto &Arg : the_function->args())
		state.scopes.back().named_values[std::string(Arg.getName())] = &Arg;

	state.current_scope().current_function = the_function;

	// todo, should I do something with the returning value of the next codegen call? probably...
	auto code = codegen(state, decl.body);
	code->insertInto(the_function);

	// Validate the generated code, checking for consistency.
	std::string err;
	llvm::raw_string_ostream err_output(err);
	if (!llvm::verifyFunction(*the_function, &err_output)) {
//		printf("Read function definition:\n");
//		the_function->print(llvm::outs());
//		printf("\n");

		state.FPM->run(*the_function);

		state.scopes.pop_back();
		return the_function;
	} else {
		// Error reading body, remove function.
		state.scopes.pop_back();
		the_function->eraseFromParent();
		state.report_error(err, decl);
		return nullptr;
	}
}

llvm::BasicBlock *codegen(codegen::state &state, ast::fn_body &body) {
	if (std::holds_alternative<ast::fn_body_block>(body)) {
		return codegen(state, std::get<ast::fn_body_block>(body));
	} else if (std::holds_alternative<ast::fn_body_expr>(body)) {
		return codegen(state, std::get<ast::fn_body_expr>(body));
	} else {
		state.report_error("unsupported body type");
		return nullptr;
	}
}

llvm::BasicBlock *codegen(codegen::state &state, ast::fn_body_expr &body) {
	llvm::BasicBlock *BB = llvm::BasicBlock::Create(*state.TheContext, "entry");
	state.Builder.SetInsertPoint(BB);
	state.Builder.CreateRet(codegen(state, body.expr));
	return BB;
}

llvm::BasicBlock *codegen(codegen::state &state, ast::fn_body_block &body) {
	llvm::BasicBlock *BB = llvm::BasicBlock::Create(*state.TheContext, "entry");
	state.Builder.SetInsertPoint(BB);

	for (auto &stmt : body.statements) {
		codegen(state, stmt);
	}

	return BB;
}

void proto_pass(codegen::state &state, ast::decl_ptr decl) {
	if (typeid(*decl) == typeid(ast::decl_fn)) {
		proto_pass(state, *(ast::decl_fn *)decl.get());
	}
}

void proto_pass(codegen::state &state, ast::decl_fn &decl) {
	// First, check for an existing function from a previous 'extern' declaration.
	//llvm::Function *the_function = state.TheModule->getFunction(decl.ident.name);

	if (state.current_scope().named_values.contains(decl.ident.name)) {
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

	state.current_scope().named_values[decl.ident.name] = the_function;
}

} // namespace catalyst::compiler::codegen
