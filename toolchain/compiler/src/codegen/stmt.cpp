// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iomanip>
#include <sstream>
#include <typeinfo>

#include "catalyst/rtti.hpp"
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
	if (isa<ast::statement_expr>(stmt)) {
		codegen(state, *(ast::statement_expr *)stmt.get());
	} else if (isa<ast::statement_decl>(stmt)) {
		codegen(state, *(ast::statement_decl *)stmt.get());
	} else if (isa<ast::statement_return>(stmt)) {
		codegen(state, *(ast::statement_return *)stmt.get());
	} else if (isa<ast::statement_if>(stmt)) {
		codegen(state, *(ast::statement_if *)stmt.get());
	} else if (isa<ast::statement_block>(stmt)) {
		codegen(state, *(ast::statement_block *)stmt.get());
	} else {
		state.report_message(report_type::error, "unsupported statement type", stmt.get());
	}
}

void codegen(codegen::state &state, ast::statement_expr &stmt) { codegen(state, stmt.expr); }

void codegen(codegen::state &state, ast::statement_return &stmt) {
	auto expecting_type = ((type_function*)state.current_function_symbol->type.get())->return_type;
	
	if (isa<type_void>(expecting_type)) {
		if (stmt.expr.has_value()) {	
			state.report_message(report_type::error, "‘return’ with a value, in function returning void", &stmt);
			return;
		}
		state.Builder.CreateBr(state.current_return_block);
	} else {
		if (!stmt.expr.has_value()) {
			state.report_message(report_type::error, "Expecting return value", &stmt);
			return;
		}
		auto expr_type = expr_resulting_type(state, stmt.expr.value(), expecting_type);
		auto return_type = ((type_function *)state.current_function_symbol->type.get())->return_type;

		if (*expr_type != *return_type) {
			state.report_message(report_type::error, "Conflicting return type", &stmt);
			std::string message = "Got ";
			message += expr_type->get_fqn();
			message += ", but expected type ";
			message += return_type->get_fqn();
			state.report_message(report_type::info, message);
			state.report_message(report_type::info, "For function starting here",
								state.current_function_symbol->ast_node);
			// TODO: warn (instead of error) about return type mismatch
		}

		codegen_assignment(state, state.current_return, return_type, stmt.expr.value());

		//auto expr = codegen(state, stmt.expr.value(), expecting_type);
		//state.Builder.CreateStore(expr, state.current_return, return_type);
		state.Builder.CreateBr(state.current_return_block);
	}
}

void codegen(codegen::state &state, std::vector<ast::statement_ptr> const &statements) {
	for (auto &stmt : statements) {
		if (state.Builder.GetInsertBlock()->getTerminator()) {
			state.report_message(report_type::warning, "Statements after return", stmt.get());
		} else {
			codegen(state, stmt);
		}
	}
} 

std::string scope_name(const ast::statement_block &stmt) {
	std::stringstream sstream;
	sstream << std::hex << (size_t)(&stmt);
	return sstream.str();
}

void codegen(codegen::state &state, ast::statement_block &stmt) {
	// llvm::BasicBlock *BB = llvm::BasicBlock::Create(*state.TheContext, "stmt_block",
	//                                                 state.current_scope().current_function);
	// state.Builder.SetInsertPoint(BB);

	state.scopes.enter(scope_name(stmt));
	codegen(state, stmt.statements);
	state.scopes.leave();
}

void codegen(codegen::state &state, ast::statement_decl &stmt) {
	codegen(state, stmt.decl);
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
		state.report_message(report_type::error,
		                     "condition does not result in comparable primitive type", stmt.cond.get());
		return;
	}

	auto cond_val =
		state.Builder.CreateICmpNE(cond_expr, p_cond_type->get_llvm_constant_zero(state), "ifcond");

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// eend of the function.
	llvm::BasicBlock *ThenBB =
		llvm::BasicBlock::Create(*state.TheContext, "then", state.current_function);
	llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*state.TheContext, "ifcont");
	llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*state.TheContext, "else");

	state.Builder.CreateCondBr(cond_val, ThenBB, ElseBB);

	// Emit then value.
	state.Builder.SetInsertPoint(ThenBB);

	codegen(state, stmt.then);
	// if (!ThenV)
	//	return;

	if (!state.Builder.GetInsertBlock()->getTerminator())
		state.Builder.CreateBr(MergeBB);
	// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
	ThenBB = state.Builder.GetInsertBlock();

	// Emit else block.
	state.current_function->getBasicBlockList().push_back(ElseBB);
	state.Builder.SetInsertPoint(ElseBB);
	if (stmt.else_.has_value()) {
		codegen(state, stmt.else_.value());
	}
	if (!state.Builder.GetInsertBlock()->getTerminator())
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
