// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <variant>

namespace catalyst::compiler::codegen {
struct symbol;
}

#include "type.hpp"

namespace catalyst::compiler::codegen {

template<class> inline constexpr bool always_false_v = false;

struct symbol {
	using ast_node_type = std::variant<ast::statement_var *, ast::fn_parameter *, ast::decl_fn *>;
	ast_node_type ast_node;
	// value can't be a smart pointer, because the destructor is protected
	llvm::Value *value = nullptr;
	std::shared_ptr<codegen::type> type;

	symbol() = default;

	symbol(ast_node_type ast_node, llvm::Value *value, std::shared_ptr<codegen::type> type)
		: ast_node(ast_node), value(value), type(std::move(type)) {}

	inline catalyst::parser::positional* get_positional() const {
		catalyst::parser::positional* pos = nullptr;
		std::visit([this, &pos](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, ast::statement_var *>) {
				auto node =  std::get<ast::statement_var *>(ast_node);
				pos = &node->ident;
			} else if constexpr (std::is_same_v<T, ast::fn_parameter *>) {
				auto node =  std::get<ast::fn_parameter *>(ast_node);
				pos = node;
			} else if constexpr (std::is_same_v<T, ast::decl_fn *>) {
				auto node =  std::get<ast::decl_fn *>(ast_node);
				pos = node;
			} else static_assert(always_false_v<T>, "non-exhaustive visitor!");
		}, ast_node);
		return pos;
	}
};

using symbol_map = std::map<std::string, symbol>;
using symbol_map_view = std::map<std::string, symbol *>;

} // namespace catalyst::compiler::codegen
