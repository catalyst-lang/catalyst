// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "decl_overloading_pass.hpp"

namespace catalyst::compiler::codegen {

int overloading_pass::process(ast::decl_fn &decl) {
    auto key = state.scopes.get_fully_qualified_scope_name(decl.ident.name);

    if (!names.contains(key)) {
        names.insert(key);
        return 0;
    }

    int i = 1;
    while (names.contains(key + "`" + std::to_string(i))) i++;
    names.insert(key + "`" + std::to_string(i));
    decl.ident.name += std::string("`") + std::to_string(i);

    return 0;
}

}
