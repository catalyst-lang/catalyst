#pragma once
#include <memory>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"
#include "pass.hpp"

namespace catalyst::compiler::codegen {

struct locals_pass : pass {
    explicit locals_pass(codegen::state &state, int n = 0) : pass(state) { this->n = n; }
    virtual int process(ast::decl_var &stmt) override;
    virtual int process(ast::statement_return &stmt) override;
    virtual int process(ast::decl_fn &decl) override;
};

}
