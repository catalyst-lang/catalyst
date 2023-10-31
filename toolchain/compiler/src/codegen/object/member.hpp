// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <string>
#include <unordered_set>

#include "catalyst/ast/ast.hpp"

namespace catalyst::compiler::codegen {
struct type;
struct type_custom;

struct member {
	member(const std::string &name, std::shared_ptr<type> type, ast::decl_ptr decl,
	       const std::vector<ast::decl_classifier> &classifiers)
		: name(name), type(type), decl(decl) {
		for (auto c : classifiers)
			this->classifiers.insert(c);
	}
	std::string name;
	std::shared_ptr<type> type;
	ast::decl_ptr decl;
	std::unordered_set<ast::decl_classifier> classifiers;

	bool is_virtual() const {
		return classifiers.contains(ast::decl_classifier::virtual_) ||
		       classifiers.contains(ast::decl_classifier::override_);
	}
};

struct member_locator {
	inline member_locator(member *member, type_custom *residence)
		: member(member), residence(residence) {}

	inline static member_locator invalid() { return member_locator(nullptr, nullptr); }

	inline bool is_valid() const { return member != nullptr && residence != nullptr; }

	member *member;
	type_custom *residence;

	std::string get_fqn() const;

	bool operator==(const member_locator& rhs) const
	{
		return member == rhs.member && residence == rhs.residence;
	}

};

} // namespace catalyst::compiler::codegen
