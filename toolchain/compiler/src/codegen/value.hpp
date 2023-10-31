// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include "../common/catalyst/ast/ast.hpp"

#include "codegen.hpp"

namespace catalyst::compiler::codegen {

llvm::Value * get_lvalue(codegen::state &state, ast::expr_ptr expr);

} // namespace catalyst::compiler::codegen
