// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "member.hpp"
#include "object_type.hpp"

namespace catalyst::compiler::codegen {

std::string member_locator::get_fqn() const { return residence->name + "." + member->name; }

} // namespace catalyst::compiler::codegen
