#include "decl_locals_pass.hpp"

#include "catalyst/rtti.hpp"
#include "expr_type.hpp"
#include "decl_type.hpp"

namespace catalyst::compiler::codegen {

/// Recursively go over all nested local variables in an AST node and add them to the symbol_table
/// @return number of locals added or changed in this pass

int locals_pass::process(ast::decl_var &stmt) {
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

int locals_pass::process(ast::statement_return &stmt) {
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

/// Perform a locals pass for a function declaration
int locals_pass::process(ast::decl_fn &decl) {
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

	state.scopes.leave();
	return pass_changes;
}

}
