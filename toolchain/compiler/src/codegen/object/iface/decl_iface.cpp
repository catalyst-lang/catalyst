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

int proto_pass::process(ast::decl_iface &decl) {
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

	if (n == 0 && state.symbol_table.contains(key)) {
		auto other = state.symbol_table[key];
		state.report_message(report_type::error, "Interface name already exists", &decl.ident);
		state.report_message(report_type::info, "Previous declaration here", other.ast_node);
		return 0;
	}

	int changed_num = n == 0 ? 1 : 0;

	auto iface_type_shared_ptr = decl_get_type(state, decl);
	auto iface_type = (type_iface *)iface_type_shared_ptr.get();
	iface_type->name = key; // TODO: is there a case where these are not the same???

	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &decl, nullptr, iface_type_shared_ptr);

	return changed_num;
}

int proto_pass::process_after(ast::decl_iface &decl) {
	int changes = 0;

	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto s = (type_iface *)sym.type.get();

	auto iface_type_shared_ptr = decl_get_type(state, decl);
	auto iface_type = (type_iface *)iface_type_shared_ptr.get();

	state.scopes.enter(decl.ident.name);

	for (auto &member : iface_type->members) {
		if (isa<type_function>(member.type)) {
			auto key = state.scopes.get_fully_qualified_scope_name(member.name);
			if (state.symbol_table.contains(key)) {
				member.type = state.symbol_table[key].type;
			}
		}
	}

	state.scopes.leave();

	if (*s != *iface_type) {
		// We can't just reassign sym.type to class_type, as there might be references to the
		// structure pointed to by sym.type at this point.
		// We only want one instance of type_class to ever exist per definition.
		s->copy_from(*iface_type);
		changes++;
	}

	// if (!s->init_function) {
	// 	// create class init function
	// 	auto *FT = llvm::FunctionType::get(llvm::Type::getVoidTy(*state.TheContext),
	// 	                                   {state.Builder.getPtrTy()}, false);
	// 	s->init_function = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
	// 	                                          key + "..__CATA_INIT", state.TheModule.get());
	// 	s->init_function->setDSOLocal(true);
	// 	changes++;
	// }

	return changes;
}

llvm::Value* codegen(codegen::state &state, ast::decl_iface &decl) {
	// Verify classifiers and report errors
	if (!check_decl_classifiers(state, decl)) { return nullptr; }
	
	auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);
	auto &sym = state.symbol_table[key];
	auto type = (type_iface *)sym.type.get();

	// auto llvm_type = type->get_llvm_type(state);

	// auto structAlloca = state.Builder.CreateAlloca(llvm_type);

	state.scopes.enter(decl.ident.name);

	for (auto &member : type->members) {
		if (isa<ast::decl_fn>(member.decl)) {
			auto fn = std::static_pointer_cast<ast::decl_fn>(member.decl);
			if (fn->body.has_value()) {
				codegen(state, member.decl);
			}
		} else {
			state.report_message(report_type::error, "Unsupported declaration type for interface", member.decl.get());
		}
	}

	state.scopes.leave();

	// Generate the metadata object
	type->get_llvm_metadata_object(state);

	return nullptr;
}

} // namespace catalyst::compiler::codegen
