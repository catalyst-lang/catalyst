// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <vector>

#include "catalyst/rtti.hpp"
#include "decl_type.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_fn &decl) {
	auto return_type =
		decl.type.has_value() ? type::create(state, decl.type.value()) : type::create_builtin();

	std::vector<std::shared_ptr<type>> params;
	for (const auto &param : decl.parameter_list) {
		if (!param.type.has_value()) {
			state.report_message(report_type::error, "Parameter has no type", &param);
			return nullptr;
		}
		params.push_back(type::create(state, param.type.value()));
	}

	return type::create_function(return_type, params);
}

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_var &decl) {
	if (decl.type.has_value()) {
		return type::create(state, decl.type.value());
	} else if (decl.expr.has_value()) {
		return expr_resulting_type(state, decl.expr.value());
	} else {
		state.report_message(report_type::error,
		                     "Global variable must have explicit type set or the type must be "
		                     "inferrable from a direct assignment",
		                     &decl);
		return nullptr;
	}
}

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_struct &decl) {
	std::vector<member> members;
	for (auto &mmbr : decl.declarations) {
		auto type = decl_get_type(state, mmbr);
		// if (auto fn = dynamic_cast<type_function*>(type.get())) {
		//	fn->is_method = true;
		// }
		members.emplace_back(mmbr->ident.name, type, mmbr, mmbr->classifiers);
	}
	return type::create_struct(
		state.current_scope().get_fully_qualified_scope_name(decl.ident.name), members);
}

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_class &decl) {
	std::vector<member> members;
	for (auto &mmbr : decl.declarations) {
		auto type = decl_get_type(state, mmbr);
		members.emplace_back(mmbr->ident.name, type, mmbr, mmbr->classifiers);
	}

	auto fqn = state.current_scope().get_fully_qualified_scope_name(decl.ident.name);
	if (!decl.super.empty()) {
		std::vector<std::shared_ptr<type_virtual>> supers;

		for (auto const &super : decl.super) {
			if (!isa<ast::type_qualified_name>(super)) {
				return type::create_class(fqn, {type_class::unknown()}, members);
			}

			auto super_qn = std::static_pointer_cast<ast::type_qualified_name>(super);
			auto super_sym = state.scopes.find_named_symbol(*super_qn);
			if (!super_sym) {
				return type::create_class(fqn, {type_class::unknown()}, members);
			}
			if (!isa<type_virtual>(super_sym->type)) {
				state.report_message(report_type::error, "Unexpected base type", super.get());
			}
			auto super_class = std::static_pointer_cast<type_virtual>(super_sym->type);
			supers.push_back(super_class);
		}

		return type::create_class(fqn, supers, members);
	} else {
		return type::create_class(fqn, members);
	}
}

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_iface &decl) {
	std::vector<member> members;
	for (auto &mmbr : decl.declarations) {
		auto type = decl_get_type(state, mmbr);
		members.emplace_back(mmbr->ident.name, type, mmbr, mmbr->classifiers);
	}

	auto fqn = state.current_scope().get_fully_qualified_scope_name(decl.ident.name);

	if (!decl.super.empty()) {
		std::vector<std::shared_ptr<type_virtual>> supers;

		for (auto const &super : decl.super) {
			if (!isa<ast::type_qualified_name>(super)) {
				return type::create_iface(fqn, {type_class::unknown()}, members);
			}

			auto super_qn = std::static_pointer_cast<ast::type_qualified_name>(super);
			auto super_sym = state.scopes.find_named_symbol(*super_qn);
			if (!super_sym) {
				return type::create_class(fqn, {type_iface::unknown()}, members);
			}
			if (!isa<type_iface>(super_sym->type)) {
				state.report_message(report_type::error, "Unexpected base type", super.get());
				state.report_message(report_type::help,
				                     "An 'iface' can only inherit from other 'iface' types.");
			}
			auto super_class = std::static_pointer_cast<type_iface>(super_sym->type);
			supers.push_back(super_class);
		}

		return type::create_iface(fqn, supers, members);
	} else {
		return type::create_iface(fqn, members);
	}
}

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, const ast::decl_ptr &decl) {
	if (isa<ast::decl_fn>(decl)) {
		return decl_get_type(state, *(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_var>(decl)) {
		return decl_get_type(state, *(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		return decl_get_type(state, *(ast::decl_struct *)decl.get());
	} else if (isa<ast::decl_class>(decl)) {
		return decl_get_type(state, *(ast::decl_class *)decl.get());
	} else if (isa<ast::decl_iface>(decl)) {
		return decl_get_type(state, *(ast::decl_iface *)decl.get());
	} else {
		state.report_message(report_type::error, "Decl type not implemented", decl.get());
		return nullptr;
	}
}

} // namespace catalyst::compiler::codegen
