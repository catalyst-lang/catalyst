// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iomanip>
#include <sstream>
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
void codegen(codegen::state &state, ast::statement_ptr stmt) {
	if (typeid(*stmt) == typeid(ast::statement_expr)) {
		codegen(state, *(ast::statement_expr *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_var)) {
		codegen(state, *(ast::statement_var *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_return)) {
		codegen(state, *(ast::statement_return *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_const)) {
		codegen(state, *(ast::statement_const *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_if)) {
		codegen(state, *(ast::statement_if *)stmt.get());
	} else if (typeid(*stmt) == typeid(ast::statement_block)) {
		codegen(state, *(ast::statement_block *)stmt.get());
	} else {
		state.report_error("unsupported statement type");
	}
}

void codegen(codegen::state &state, ast::statement_expr &stmt) { codegen(state, stmt.expr); }

void codegen(codegen::state &state, ast::statement_return &stmt) {
	auto expr = codegen(state, stmt.expr);
	auto expr_type = expr_resulting_type(state, stmt.expr);

	if (*expr_type != *((type_function*)state.current_function_symbol->type.get())->return_type) {
		state.report_error("Mixed return types", *state.current_function_symbol->ast_node);
		// TODO: warn (instead of error) about return type mismatch
	}

	state.Builder.CreateStore(expr, state.current_return);
}

void codegen(codegen::state &state, ast::statement_var &stmt) {
	// the locals pass already made sure there is a value in the symbol table
	auto var = state.scopes.find_named_symbol(stmt.ident.name);
	if (stmt.expr.has_value() && stmt.expr.value() != nullptr) {
		codegen_assignment(state, var->value, var->type, stmt.expr.value());
	}
}

void codegen(codegen::state &state, ast::statement_const &stmt) {
	state.report_error("statement_const: Not implemented");
}

void codegen(codegen::state &state, ast::statement_block &stmt) {
	// llvm::BasicBlock *BB = llvm::BasicBlock::Create(*state.TheContext, "stmt_block",
	//                                                 state.current_scope().current_function);
	// state.Builder.SetInsertPoint(BB);

	std::stringstream sstream;
	sstream << std::hex << (size_t)(&stmt);
	state.scopes.enter(sstream.str());
	for (auto &stmt : stmt.statements) {
		codegen(state, stmt);
	}
	state.scopes.leave();
}

void codegen(codegen::state &state, ast::statement_if &stmt) {
	auto cond_expr = codegen(state, stmt.cond);
	if (!cond_expr)
		return;

	// Convert condition to a bool by comparing non-equal to 0.0.
	// cond_val = state.Builder.CreateFCmpONE(
	//	cond_val, llvm::ConstantFP::get(*state.TheContext, llvm::APFloat(0.0)), "ifcond");

	auto cond_type = expr_resulting_type(state, stmt.cond);
	auto p_cond_type = dynamic_cast<type_primitive *>(cond_type.get());
	if (p_cond_type == nullptr) {
		state.report_error("condition does not result in comparable primitive type");
		return;
	}

	auto cond_val =
		state.Builder.CreateICmpNE(cond_expr, p_cond_type->get_llvm_constant_zero(state), "ifcond");

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// eend of the function.
	llvm::BasicBlock *ThenBB =
		llvm::BasicBlock::Create(*state.TheContext, "then", state.current_function);
	llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*state.TheContext, "else");
	llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*state.TheContext, "ifcont");

	state.Builder.CreateCondBr(cond_val, ThenBB, ElseBB);

	// Emit then value.
	state.Builder.SetInsertPoint(ThenBB);

	codegen(state, stmt.then);
	// if (!ThenV)
	//	return;

	state.Builder.CreateBr(MergeBB);
	// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
	ThenBB = state.Builder.GetInsertBlock();

	// Emit else block.
	state.current_function->getBasicBlockList().push_back(ElseBB);
	state.Builder.SetInsertPoint(ElseBB);

	codegen(state, stmt.else_.value());
	// if (!ElseV)
	//	return;

	state.Builder.CreateBr(MergeBB);
	// codegen of 'Else' can change the current block, update ElseBB for the PHI.
	ElseBB = state.Builder.GetInsertBlock();

	// Emit merge block.
	state.current_function->getBasicBlockList().push_back(MergeBB);
	state.Builder.SetInsertPoint(MergeBB);
	/*llvm::PHINode *PN =
	    state.Builder.CreatePHI(llvm::Type::getInt64Ty(*state.TheContext), 2, "iftmp");

	PN->addIncoming(ThenV, ThenBB);
	PN->addIncoming(ElseV, ElseBB);
	return PN;*/
	// return MergeBB;
}

} // namespace catalyst::compiler::codegen
