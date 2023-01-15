// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"
#include "pass.hpp"

namespace catalyst::compiler::codegen {

struct overloading_pass : pass {
    explicit overloading_pass(codegen::state &state) : pass(state) {}
    int process(ast::decl_fn &decl) override;

    private:
    std::unordered_set<std::string> names;
};
} // namespace catalyst::compiler::codegen
