// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "catalyst/ast/expr.hpp"
#include <memory>
#include <string>

namespace catalyst::compiler::codegen {
struct type;
}

namespace catalyst::compiler::codegen {

struct member {
	member(const std::string &name, std::shared_ptr<type> type, ast::decl_ptr decl)
		: name(name), type(type), decl(decl) {}
	std::string name;
	std::shared_ptr<type> type;
	ast::decl_ptr decl;
};

struct member_locator {
	inline member_locator(member *member, type_custom *residence)
		: member(member), residence(residence) {}

	inline static member_locator invalid() { return member_locator(nullptr, nullptr); }

	inline bool is_valid() const { return member != nullptr && residence != nullptr; }

	member *member;
	type_custom *residence;
};

} // namespace catalyst::compiler::codegen
