#pragma once
#include <memory>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

int locals_pass(codegen::state &state, int n, ast::decl_ptr &decl);
int locals_pass(codegen::state &state, int n, ast::statement_decl &stmt);
int locals_pass(codegen::state &state, int n, ast::decl_var &stmt);
int locals_pass(codegen::state &state, int n, ast::statement_if &stmt);
int locals_pass(codegen::state &state, int n, ast::statement_block &stmt);
int locals_pass(codegen::state &state, int n, ast::statement_return &stmt);
int locals_pass(codegen::state &state, int n, ast::statement_ptr &stmt);
int locals_pass(codegen::state &state, int n, ast::decl_fn &decl);
int locals_pass(codegen::state &state, int n, ast::decl_ptr &decl);

}
