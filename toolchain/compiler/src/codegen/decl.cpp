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
llvm::Value* codegen(codegen::state &state, ast::decl_ptr decl) {
	if (isa<ast::decl_fn>(decl)) {
		return codegen(state, *(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_var>(decl)) {
		return codegen(state, *(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		return codegen(state, *(ast::decl_struct *)decl.get());
	} else if (isa<ast::decl_class>(decl)) {
		return codegen(state, *(ast::decl_class *)decl.get());
	} else if (isa<ast::decl_ns>(decl)) {
		return codegen(state, *(ast::decl_ns *)decl.get());
	} else {
		state.report_message(report_type::error, "Decl type not implemented", decl.get());
		return nullptr;
	}
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

llvm::Value* codegen(codegen::state &state, ast::decl_fn &decl) {
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
			} else if (isa<type_class>(to->object_type)) {
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
		the_function = nullptr;
	}

	state.current_function = previous_function;
	state.current_function_symbol = previous_function_symbol;
	state.current_return = previous_return;
	state.current_return_block = previous_return_block;

	return the_function;
}

llvm::Value* codegen(codegen::state &state, ast::fn_body &body) {
	if (std::holds_alternative<ast::fn_body_block>(body)) {
		return codegen(state, std::get<ast::fn_body_block>(body));
	} else if (std::holds_alternative<ast::fn_body_expr>(body)) {
		return codegen(state, std::get<ast::fn_body_expr>(body));
	} else {
		state.report_message(report_type::error, "unsupported body type");
		return nullptr;
	}
}

llvm::Value* codegen(codegen::state &state, ast::fn_body_expr &body) {
	auto expr = codegen(state, body.expr);
	return state.Builder.CreateRet(expr);
}

llvm::Value* codegen(codegen::state &state, ast::fn_body_block &body) { 
	codegen(state, body.statements);
	return nullptr;
}

llvm::Value* codegen(codegen::state &state, ast::decl_var &decl) {
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
	return var->value;
}

llvm::Value* codegen(codegen::state &state, ast::decl_ns &decl) {
	if (decl.is_global) return nullptr; // this is handled in the translation unit

	state.scopes.enter(decl.ident.name);

	for (const auto &decl : decl.declarations) {	
		state.Builder.SetInsertPoint(&state.init_function->getEntryBlock());
		
		codegen(state, decl);
	}
	
	state.scopes.leave();

	return nullptr;
}

} // namespace catalyst::compiler::codegen
