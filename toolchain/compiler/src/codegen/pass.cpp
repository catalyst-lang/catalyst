#include "pass.hpp"

#include "catalyst/rtti.hpp"
#include "stmt.hpp"

namespace catalyst::compiler::codegen {

int pass::operator()(ast::decl_ptr &decl) {
	int res = walk(decl);
	n++;
	return res;
}

int pass::operator()(ast::translation_unit &tu) {
	int res = walk(tu);
	n++;
	return res;
}

int pass::walk(ast::translation_unit &tu) {
	int res = process(tu);
	for (auto &decl : tu.declarations) {
		res += walk(decl);
	}
	res += process_after(tu);
	return res;
}

int pass::walk(ast::decl_ptr &decl) {
	if (isa<ast::decl_var>(decl)) {
		return walk(*(ast::decl_var *)decl.get());
	} else if (isa<ast::decl_fn>(decl)) {
		return walk(*(ast::decl_fn *)decl.get());
	} else if (isa<ast::decl_struct>(decl)) {
		return walk(*(ast::decl_struct *)decl.get());
	}
	state.report_message(report_type::error, "Choices exhausted", decl.get());
	return 0;
}

int pass::walk(ast::decl_fn &decl) {
	int res = process(decl);
	state.scopes.enter(decl.ident.name);
	if (std::holds_alternative<ast::fn_body_block>(decl.body)) {
		auto &block = std::get<ast::fn_body_block>(decl.body);
		for (auto &stmt : block.statements) {
			res += walk(stmt);
		}
	} else if (std::holds_alternative<ast::fn_body_expr>(decl.body)) {
		//auto &expr = std::get<ast::fn_body_expr>(decl.body);
		//res += walk(expr.expr);
	}
	state.scopes.leave();
	res += process_after(decl);
	return res;
}

int pass::walk(ast::decl_var &decl) {
	return process(decl) + process_after(decl);
}

int pass::walk(ast::decl_struct &decl) {
	int res = process(decl);
	state.scopes.enter(decl.ident.name);
	for (auto &decl : decl.declarations) {
		res += walk(decl);
	}
	state.scopes.leave();
	res += process_after(decl);
	return res;
}

int pass::walk(ast::statement_ptr &stmt) {
	if (isa<ast::statement_decl>(stmt)) {
		return walk(*(ast::statement_decl *)stmt.get());
	} else if (isa<ast::statement_if>(stmt)) {
		return walk(*(ast::statement_if *)stmt.get());
	} else if (isa<ast::statement_block>(stmt)) {
		return walk(*(ast::statement_block *)stmt.get());
	} else if (isa<ast::statement_return>(stmt)) {
		return walk(*(ast::statement_return *)stmt.get());
	} else if (isa<ast::statement_for>(stmt)) {
		return walk(*(ast::statement_for *)stmt.get());
	} else if (isa<ast::statement_expr>(stmt)) {
		return walk(*(ast::statement_expr *)stmt.get());
	}
	state.report_message(report_type::error, "Choices exhausted", stmt.get());
	return 0;
}


int pass::walk(ast::statement_decl &stmt) {
	return process(stmt) + walk(stmt.decl) + process_after(stmt);
}

int pass::walk(ast::statement_expr &stmt) {
	return process(stmt) + process_after(stmt);
}

int pass::walk(ast::statement_return &stmt) {
	return process(stmt) + process_after(stmt);
}

int pass::walk(ast::statement_block &stmt) {
	int res = process(stmt);
	state.scopes.enter(scope_name(stmt));
	for (auto &stmt : stmt.statements) {
		res += walk(stmt);
	}
	state.scopes.leave();
	res += process_after(stmt);
	return res;

}

int pass::walk(ast::statement_if &stmt) {
	int res = process(stmt);
	res += walk(stmt.then);
	if (stmt.else_.has_value()) {
		res += walk(stmt.else_.value());
	}
	res += process_after(stmt);
	return res;
}

int pass::walk(ast::statement_for &stmt) {
	return process(stmt) + walk(stmt.body) + process_after(stmt);
}



}
