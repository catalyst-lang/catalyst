#include "decl_locals_pass.hpp"

#include "catalyst/rtti.hpp"
#include "expr_type.hpp"
#include "decl_type.hpp"

namespace catalyst::compiler::codegen {

/// Recursively go over all nested local variables in an AST node and add them to the symbol_table
/// @return number of locals added or changed in this pass
int c;

int locals_pass(codegen::state &state, int n,
                std::vector<catalyst::ast::statement_ptr> &statements) {
	int locals_changed = 0;
	for (auto &stmt : statements) {
		locals_changed += locals_pass(state, n, stmt);
	}
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_decl &stmt) {
	return locals_pass(state, n, stmt.decl);
}

int locals_pass(codegen::state &state, int n, ast::decl_var &stmt) {
	auto key = state.scopes.get_fully_qualified_scope_name(stmt.ident.name);
	int locals_changed = 0;

	// get the existing symbol OR emplace a new symbol
	const auto [res, symbol_introduced] =
		state.symbol_table.try_emplace(key, &stmt, nullptr, type::create_builtin());
	auto &sym = res->second;

	// if first pass and symbol is NOT introduced, this is a redefinition error
	if (n == 0 && !symbol_introduced) {
		state.report_message(report_type::error, "Symbol redefined", &stmt.ident);
		return 0;
		// todo: show where first definition is
		// state.report_message("First definition here", sym.ast_node);
	}

	if (symbol_introduced)
		locals_changed = 1;

	std::shared_ptr<type> new_type;

	if (stmt.type.has_value()) {
		new_type = type::create(state, stmt.type.value());
	} else if (stmt.expr.has_value()) {
		new_type = expr_resulting_type(state, stmt.expr.value());
	} else {
		// TODO: search for first assignment and infer type
		return 0;
	}

	if (*sym.type != *new_type) {
		sym.type = new_type;
		locals_changed = 1;
	}
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_if &stmt) {
	int locals_changed = locals_pass(state, n, stmt.then);
	if (stmt.else_.has_value())
		locals_changed += locals_pass(state, n, stmt.else_.value());
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_block &stmt) {
	std::stringstream sstream;
	sstream << std::hex << (size_t)(&stmt);
	state.scopes.enter(sstream.str());
	int locals_changed = locals_pass(state, n, stmt.statements);
	state.scopes.leave();
	return locals_changed;
}

int locals_pass(codegen::state &state, int n, ast::statement_return &stmt) {
	state.current_function_has_return = true;

	std::shared_ptr<type> expr_type;
	if (stmt.expr.has_value()) {
		expr_type = expr_resulting_type(state, stmt.expr.value());
	} else {
		expr_type = type::create(state, "void");
	}

	auto fn_type = (type_function *)state.current_function_symbol->type.get();

	if (!fn_type->return_type->is_valid() && expr_type->is_valid()) {
		fn_type->return_type = expr_type;
		return 1;
	}
    
    return 0;
}

int locals_pass(codegen::state &state, int n, ast::statement_ptr &stmt) {
	if (isa<ast::statement_decl>(stmt)) {
		return locals_pass(state, n, *(ast::statement_decl *)stmt.get());
	} else if (isa<ast::statement_if>(stmt)) {
		return locals_pass(state, n, *(ast::statement_if *)stmt.get());
	} else if (isa<ast::statement_block>(stmt)) {
		return locals_pass(state, n, *(ast::statement_block *)stmt.get());
	} else if (isa<ast::statement_return>(stmt)) {
		return locals_pass(state, n, *(ast::statement_return *)stmt.get());
	} else if (isa<ast::statement_expr>(stmt)) {
		// TODO
		return 0;
	}
	state.report_message(report_type::error, "Choices exhausted", stmt.get());
	return 0;
}

/// Perform a locals pass for a function declaration
int locals_pass(codegen::state &state, int n, ast::decl_fn &decl) {
	state.scopes.enter(decl.ident.name);

	int pass_changes = 0;
	if (n == 0) {
		for (auto &param : decl.parameter_list) {
			if (!param.type.has_value()) {
				state.report_message(report_type::error, "Parameter has no type", &param);
				return 0;
			}
			auto key = state.scopes.get_fully_qualified_scope_name(param.ident.name);
			state.symbol_table[key] =
				symbol(&param, nullptr, codegen::type::create(state, param.type.value()));
			pass_changes++;
		}
	}

	if (std::holds_alternative<ast::fn_body_block>(decl.body)) {
		auto &block = std::get<ast::fn_body_block>(decl.body);
		pass_changes = locals_pass(state, n, block.statements);
	}

	state.scopes.leave();
	return pass_changes;
}

int locals_pass(codegen::state &state, int n, ast::decl_ptr &decl) {
	if (isa<ast::decl_var>(decl)) {
		return locals_pass(state, n, *(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_fn>(decl)) {
		return locals_pass(state, n, *(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		state.report_message(report_type::error, "Local structs not supported (yet)", decl.get());
		return 0;
		// return locals_pass(state, n, *(ast::decl_struct *)decl.get());
	}
	state.report_message(report_type::error, "Choices exhausted", decl.get());
	return 0;
}

}
