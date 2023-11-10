// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <memory>
#include <string>

#include "decl.hpp"
#include "decl_type.hpp"

namespace catalyst::compiler::codegen {

std::string classifier_to_string(ast::decl_classifier c);
ast::decl_classifier string_to_classifier(const std::string &str);

} // namespace catalyst::compiler::codegen
