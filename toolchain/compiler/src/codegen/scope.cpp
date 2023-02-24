// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "object/object_type.hpp"
#include "scope.hpp"

namespace catalyst::compiler::codegen {

	std::set<symbol *> scope_stack::find_overloaded_symbol(const std::string &name, bool exact_match, bool look_in_multiple_scopes) {
		std::set<symbol *> results;

		if (!exact_match)
			for (auto it = rbegin(); it != rend(); ++it) {
				int i = 1;
				auto potential_local_name = (*it).get_fully_qualified_scope_name(name);
				auto key = potential_local_name;
				while (symbol_table->contains(key)) {
					results.insert(&(*symbol_table)[key]);
					key = potential_local_name + "`" + std::to_string(i++);
				}
				if (!look_in_multiple_scopes && results.size() > 0) break;
			}
		if (symbol_table->contains(name)) {
			int i = 1;
			auto key = name;
			while (symbol_table->contains(key)) {
				results.insert(&(*symbol_table)[key]);
				key = name + "`" + std::to_string(i++);
			}
		}
		// find virtual members that are not overloaded
		for (const auto &symbol : results) {
			if (auto tf = std::dynamic_pointer_cast<type_function>(symbol->type)) {
				if (tf->is_virtual()) {
					auto cl = std::dynamic_pointer_cast<type_class>(tf->method_of);
					auto tf_member = cl->get_member(tf.get());
					auto vmems = cl->get_virtual_members(tf_member.member->name);
					for (const auto& vmem : vmems) {
						results.insert(&(*symbol_table)[vmem.get_fqn()]);
					}
				}
			}
		}

		return results;
	}

} // namespace catalyst::compiler::codegen
