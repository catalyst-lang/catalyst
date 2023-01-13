// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#include <memory>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

struct pass {
    explicit pass(codegen::state &state) : state(state) {}

    int operator()(ast::decl *decl);
    int operator()(ast::decl_ptr &decl);
    int operator()(ast::translation_unit &tu);

    protected:
    codegen::state &state;
    int n = 0;

    virtual int process(ast::translation_unit &tu) { return 0; };
    virtual int process(ast::decl_fn &decl) { return 0; };
    virtual int process(ast::decl_var &decl) { return 0; };
    virtual int process(ast::decl_struct &decl) { return 0; };
    virtual int process(ast::statement_decl &decl) { return 0; };
    virtual int process(ast::statement_expr &decl) { return 0; };
    virtual int process(ast::statement_return &decl) { return 0; };
    virtual int process(ast::statement_block &decl) { return 0; };
    virtual int process(ast::statement_if &decl) { return 0; };
    virtual int process(ast::statement_for &decl) { return 0; };

    virtual int process_after(ast::translation_unit &tu) { return 0; };
    virtual int process_after(ast::decl_fn &decl) { return 0; };
    virtual int process_after(ast::decl_var &decl) { return 0; };
    virtual int process_after(ast::decl_struct &decl) { return 0; };
    virtual int process_after(ast::statement_decl &decl) { return 0; };
    virtual int process_after(ast::statement_expr &decl) { return 0; };
    virtual int process_after(ast::statement_return &decl) { return 0; };
    virtual int process_after(ast::statement_block &decl) { return 0; };
    virtual int process_after(ast::statement_if &decl) { return 0; };
    virtual int process_after(ast::statement_for &decl) { return 0; };

    private:
    int walk(ast::translation_unit &tu);
    int walk(ast::decl *decl);
    int walk(ast::decl_ptr &decl);
    int walk(ast::decl_fn &decl);
    int walk(ast::decl_var &decl);
    int walk(ast::decl_struct &decl);
    int walk(ast::statement_ptr &stmt);
    int walk(ast::statement_decl &stmt);
    int walk(ast::statement_expr &stmt);
    int walk(ast::statement_return &stmt);
    int walk(ast::statement_block &stmt); // not sure how useful this one is
    int walk(ast::statement_if &stmt);
    int walk(ast::statement_for &stmt);
};

}
