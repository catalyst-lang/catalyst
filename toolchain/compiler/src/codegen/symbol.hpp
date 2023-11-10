// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <variant>

#include "llvm/IR/Value.h"

#include "../common/catalyst/ast/ast.hpp"
#include "../serializable.hpp"

namespace catalyst::compiler::codegen {

struct type;
struct state;

template<class> inline constexpr bool always_false_v = false;

struct symbol : serializable::ISerializable {
	parser::ast_node* ast_node;
	// value can't be a smart pointer, because the destructor is protected
	llvm::Value *value = nullptr;
	std::shared_ptr<codegen::type> type;
	bool imported = false;

	symbol() = default;

	symbol(parser::ast_node* ast_node, llvm::Value *value, std::shared_ptr<codegen::type> type)
		: ast_node(ast_node), value(value), type(std::move(type)) {}

	void serialize(std::ostream& out) const override;
	static symbol deserialize(state &state, std::istream& in);
};

using symbol_map = std::map<std::string, symbol>;
using symbol_map_view = std::map<std::string, symbol *>;

} // namespace catalyst::compiler::codegen
