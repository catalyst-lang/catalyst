// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <typeinfo>

#include "catalyst/rtti.hpp"

#include "../../decl.hpp"
#include "../../decl_proto_pass.hpp"
#include "../../decl_type.hpp"
#include "../../expr.hpp"
#include "../../expr_type.hpp"

namespace catalyst::compiler::codegen {

int proto_pass::process(ast::decl_struct &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Struct name already exists", &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

	auto struct_type_sptr = decl_get_type(state, decl);
	auto struct_type = (type_struct *)struct_type_sptr.get();
	struct_type->name = key;

	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &decl, nullptr, struct_type_sptr);

	return changed_num;
}

int proto_pass::process_after(ast::decl_struct &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto s = (type_struct *)sym.type.get();

	auto struct_type_sptr = decl_get_type(state, decl);
	auto struct_type = (type_struct *)struct_type_sptr.get();

	state.scopes.enter(decl.ident.name);

	for (auto &member : struct_type->members) {
		if (isa<type_function>(member.type)) {
			member.type =
				state.symbol_table[state.scopes.get_fully_qualified_scope_name(member.name)].type;
		}
	}

	state.scopes.leave();

	if (*s != *struct_type) {
		// We can't just reassign sym.type to struct_type, as there might be references to the
		// structure pointed to by sym.type at this point.
		// We only want one instance of type_struct to ever exist per definition.
		s->copy_from(*struct_type);
		return 1;
	}

	if (!s->init_function) {
		// create class init function
		auto *FT = llvm::FunctionType::get(llvm::Type::getVoidTy(*state.TheContext),
		                                   {state.Builder.getPtrTy()}, false);
		s->init_function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
		                                          key + "..__CATA_INIT", state.TheModule.get());
		s->init_function->setDSOLocal(true);
	}

	return 0;
}

llvm::Value* codegen(codegen::state &state, ast::decl_struct &decl) {
	// Verify classifiers and report errors
	if (!check_decl_classifiers(state, decl)) { return nullptr; }
	
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto type = (type_struct *)sym.type.get();

	// auto llvm_type = type->get_llvm_type(state);

	// auto structAlloca = state.Builder.CreateAlloca(llvm_type);

	// create struct init function
	auto this_ = type->init_function->getArg(0);
	auto *BB = llvm::BasicBlock::Create(*state.TheContext, "init", type->init_function);

	state.scopes.enter(decl.ident.name);

	for (auto &member : type->members) {
		if (isa<type_function>(member.type)) {
			codegen(state, member.decl);
		} else if (isa<ast::decl_var>(member.decl)) {
			state.Builder.SetInsertPoint(BB);
			auto decl = (ast::decl_var *)member.decl.get();

			auto member_loc = type->get_member(member.name);
			auto ptr =
				state.Builder.CreateStructGEP(member_loc.residence->get_llvm_type(state), this_,
			                                  member_loc.residence->get_member_index_in_llvm_struct(member_loc));

			if (decl->expr.has_value() && decl->expr.value() != nullptr) {
				codegen_assignment(state, ptr, member.type, decl->expr.value());
			} else {
				// set default value
				auto default_val = member.type->get_default_llvm_value(state);
				if (default_val) {
					state.Builder.CreateStore(default_val, ptr);
				}
			}
		}
	}

	state.Builder.SetInsertPoint(BB);
	state.Builder.CreateRetVoid();

	state.scopes.leave();

	if (!state.TheFPM->isEmpty()) {
		state.TheFPM->run(*type->init_function, *state.TheFAM);
	}
	

	return nullptr;
}

} // namespace catalyst::compiler::codegen
