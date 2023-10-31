// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

#include "../common/catalyst/ast/ast.hpp"

#include "codegen.hpp"
#include "pass.hpp"

namespace catalyst::compiler::codegen {

struct locals_pass : pass {
    explicit locals_pass(codegen::state &state, int n = 0) : pass(state) { this->n = n; }
    int process(ast::decl_var &stmt) override;
    int process(ast::statement_return &stmt) override;
    int process(ast::fn_body_expr &body) override;
    int process(ast::decl_fn &decl) override;
};

}
