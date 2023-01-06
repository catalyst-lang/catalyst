// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include "../common/catalyst/ast/ast.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_fn &decl);
std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_var &decl);
std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_struct &decl);
std::shared_ptr<codegen::type> decl_get_type(codegen::state &state, ast::decl_ptr decl);

} // namespace catalyst::compiler::codegen
