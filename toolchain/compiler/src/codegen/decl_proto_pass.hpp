#pragma once
#include <memory>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"
#include "pass.hpp"

namespace catalyst::compiler::codegen {

struct proto_pass : pass {
    explicit proto_pass(codegen::state &state) : pass(state) {}
    int process(ast::decl_fn &decl) override;
    int process(ast::decl_var &decl) override;
    int process(ast::decl_struct &decl) override;
    int process_after(ast::decl_struct &decl) override;
};

}
