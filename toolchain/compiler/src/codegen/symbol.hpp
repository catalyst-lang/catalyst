// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <variant>
#include "../common/catalyst/ast/ast.hpp"

namespace catalyst::compiler::codegen {
struct symbol;
}

#include "type.hpp"

namespace catalyst::compiler::codegen {

template<class> inline constexpr bool always_false_v = false;

struct symbol {
	parser::ast_node* ast_node;
	// value can't be a smart pointer, because the destructor is protected
	llvm::Value *value = nullptr;
	std::shared_ptr<codegen::type> type;

	symbol() = default;

	symbol(parser::ast_node* ast_node, llvm::Value *value, std::shared_ptr<codegen::type> type)
		: ast_node(ast_node), value(value), type(std::move(type)) {}

};

using symbol_map = std::map<std::string, symbol>;
using symbol_map_view = std::map<std::string, symbol *>;

} // namespace catalyst::compiler::codegen
