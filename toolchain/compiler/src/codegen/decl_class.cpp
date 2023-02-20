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
#include "decl_proto_pass.hpp"
#include "decl_type.hpp"
#include "expr.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

int proto_pass::process(ast::decl_class &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	if (n == 0 &&
	    state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Class name already exists", &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

    auto class_type_shared_ptr = decl_get_type(state, decl);
    auto class_type = (type_class*)class_type_shared_ptr.get();
	class_type->name = key;

	const auto [res, symbol_introduced] = state.symbol_table.try_emplace(key, &decl, nullptr, class_type_shared_ptr);

	return changed_num;
}

int proto_pass::process_after(ast::decl_class &decl) {
	int changes = 0;

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
    auto s = (type_class*)sym.type.get();

	auto class_type_shared_ptr = decl_get_type(state, decl);
    auto class_type = (type_class*)class_type_shared_ptr.get();

	state.scopes.enter(decl.ident.name);

	for (auto &member : class_type->members) {
		if (isa<type_function>(member.type)) {
			member.type = state.symbol_table[state.scopes.get_fully_qualified_scope_name(member.name)].type;
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
		auto *FT = llvm::FunctionType::get(llvm::Type::getVoidTy(*state.TheContext), { state.Builder.getPtrTy() }, false);
		s->init_function = 
			llvm::Function::Create(FT, llvm::Function::ExternalLinkage, key + "..__CATA_INIT", state.TheModule.get());
		s->init_function->setDSOLocal(true);
		changes++;
	}

	// Super class (inheritance)
	if (decl.super.has_value()) {
		auto super_name = decl.super.value().ident.name;
		auto super_sym = state.scopes.find_named_symbol(super_name);
		if (!super_sym) {
			state.report_message(report_type::error, std::string("Undefined class `") + super_name + "`", &decl.super.value());
			return 0;
		}
		if (!isa<type_class>(super_sym->type)) {
			state.report_message(report_type::error, std::string("`") + super_name + "` is not a valid parent class", &decl.super.value());
			return 0;
		}
		if (s->super.get() != super_sym->type.get()) {
			s->super = std::static_pointer_cast<type_class>(super_sym->type);
			changes++;
		}
	}

	return changes;
}

void codegen(codegen::state &state, ast::decl_class &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto type = (type_class *)sym.type.get();

    // auto llvm_type = type->get_llvm_type(state);

    // auto structAlloca = state.Builder.CreateAlloca(llvm_type);

	// create class init function
	auto this_ = type->init_function->getArg(0);
	auto *BB = llvm::BasicBlock::Create(*state.TheContext, "init", type->init_function);
	
	state.scopes.enter(decl.ident.name);

	for (auto &member : type->members) {
		if (isa<type_function>(member.type)) {
			codegen(state, member.decl);
		} else if (isa<ast::decl_var>(member.decl)) {
			state.Builder.SetInsertPoint(BB);
			auto decl = (ast::decl_var*)member.decl.get();

			int index = type->index_of(member.name);
			auto ptr = state.Builder.CreateStructGEP(type->get_llvm_struct_type(state), this_, index);

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

	state.FPM->run(*type->init_function);

}


} // namespace catalyst::compiler::codegen
