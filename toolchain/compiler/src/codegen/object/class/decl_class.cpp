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

int proto_pass::process(ast::decl_class &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Class name already exists", &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

	auto class_type_shared_ptr = decl_get_type(state, decl);
	auto class_type = (type_class *)class_type_shared_ptr.get();
	class_type->name = key; // TODO: is there a case where these are not the same???

	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &decl, nullptr, class_type_shared_ptr);

	return changed_num;
}

int proto_pass::process_after(ast::decl_class &decl) {
	int changes = 0;

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto s = (type_class *)sym.type.get();

	auto class_type_shared_ptr = decl_get_type(state, decl);
	auto class_type = (type_class *)class_type_shared_ptr.get();

	state.scopes.enter(decl.ident.name);

	for (auto &member : class_type->members) {
		if (isa<type_function>(member.type)) {
			auto key = state.scopes.get_fully_qualified_scope_name(member.name);
			if (state.symbol_table.contains(key)) {
				member.type = state.symbol_table[key].type;
			}
		}
	}

	state.scopes.leave();

	if (*s != *class_type) {
		// We can't just reassign sym.type to class_type, as there might be references to the
		// structure pointed to by sym.type at this point.
		// We only want one instance of type_class to ever exist per definition.
		s->copy_from(*class_type);
		changes++;
	}

	if (!s->init_function) {
		// create class init function
		auto *FT = llvm::FunctionType::get(llvm::Type::getVoidTy(*state.TheContext),
		                                   {state.Builder.getPtrTy()}, false);
		s->init_function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
		                                          key + "..__CATA_INIT", state.TheModule.get());
		s->init_function->setDSOLocal(true);
		changes++;
	}

	return changes;
}

llvm::Value* codegen(codegen::state &state, ast::decl_class &decl) {
	// Verify classifiers and report errors
	if (!check_decl_classifiers(state, decl)) { return nullptr; }
	
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto type = (type_class *)sym.type.get();

	// auto llvm_type = type->get_llvm_type(state);

	// auto structAlloca = state.Builder.CreateAlloca(llvm_type);

	// create class init function
	auto this_ = type->init_function->getArg(0);
	auto *BB = llvm::BasicBlock::Create(*state.TheContext, "init", type->init_function);

	state.scopes.enter(decl.ident.name);

	state.Builder.SetInsertPoint(BB);

	auto metadata_location = state.Builder.CreateConstGEP1_32(llvm::PointerType::get(*state.TheContext, 0), this_, 0);
	state.Builder.CreateStore(type->get_llvm_metadata_object(state), metadata_location);

	std::function<void(type_virtual*, type_virtual*, llvm::Value*)> call_inits = [&](type_virtual *base, type_virtual *super, llvm::Value *pointer) {
		int super_index = 1; // offset by 1 for the metadata
		for (auto & s : super->super) {
			if (s->init_function != nullptr) {
				auto offsetted = state.Builder.CreateStructGEP(super->get_llvm_struct_type(state), pointer, super_index, s->name + "_offset");
				state.Builder.CreateCall(s->init_function, {offsetted});
			
				// overwrite the super metadata with our own
				state.Builder.CreateStore(base->get_llvm_metadata_object(state, *s), offsetted);
				call_inits(base, s.get(), offsetted);
			}
			super_index++;
		}
	};
	call_inits(type, type, this_);

	// int super_index = 1; // offset by 1 for the metadata
	// for (auto & s : type->super) {
	// 	if (s->init_function != nullptr) {
	// 		auto offsetted = state.Builder.CreateStructGEP(type->get_llvm_struct_type(state), this_, super_index, s->name + "_offset");
	// 		state.Builder.CreateCall(s->init_function, {offsetted});
		
	// 		// overwrite the super metadata with our own
	// 		state.Builder.CreateStore(type->get_llvm_metadata_object(state, *s), offsetted);
	// 	}
	// 	super_index++;
	// }

	for (auto &member : type->members) {
		if (isa<ast::decl_fn>(member.decl)) {
			codegen(state, member.decl);
		} else if (isa<ast::decl_var>(member.decl)) {
			state.Builder.SetInsertPoint(BB);
			auto decl = (ast::decl_var *)member.decl.get();

			auto member_loc = type->get_member(member.name);
			auto ptr = state.Builder.CreateStructGEP(
				member_loc.residence->get_llvm_struct_type(state), this_,
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

	// Generate the metadata object
	type->get_llvm_metadata_object(state); // UGLY: this is a side effect

	std::string err;
	llvm::raw_string_ostream err_output(err);
	if (!llvm::verifyFunction(*type->init_function, &err_output)) {
		if (!state.TheFPM->isEmpty()) {
			state.TheFPM->run(*type->init_function, *state.TheFAM);
		}
	} else {
		// Error reading body, remove function.
		state.report_message(report_type::error, err, &decl);
		type->init_function->print(llvm::errs());
		type->init_function->eraseFromParent();
		type->init_function = nullptr;
	}

	return nullptr;
}

} // namespace catalyst::compiler::codegen
