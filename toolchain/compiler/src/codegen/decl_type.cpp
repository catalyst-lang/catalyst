#include <vector>

#include "catalyst/rtti.hpp"
#include "decl_type.hpp"
#include "expr_type.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_fn &decl) {
	auto return_type = decl.type.has_value() ? type::create(state, decl.type.value()) : type::create_builtin();

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
		//if (auto fn = dynamic_cast<type_function*>(type.get())) {
		//	fn->is_method = true;
		//}
		members.emplace_back(mmbr->ident.name, type, mmbr);
	}
    return type::create_struct(decl.ident.name, members);
}

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_class &decl) {
    std::vector<member> members;
	for (auto &mmbr : decl.declarations) {
		auto type = decl_get_type(state, mmbr);
		//if (auto fn = dynamic_cast<type_function*>(type.get())) {
		//	fn->is_method = true;
		//}
		members.emplace_back(mmbr->ident.name, type, mmbr);
	}
    return type::create_class(decl.ident.name, members);
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
	} else {
		state.report_message(report_type::error, "Decl type not implemented", decl.get());
        return nullptr;
	}
}

} // namespace catalyst::compiler::codegen
