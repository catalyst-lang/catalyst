// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <memory>
#include <string>
#include <vector>

#include "catalyst/rtti.hpp"
#include "decl.hpp"
#include "decl_type.hpp"

namespace catalyst::compiler::codegen {

std::string classifier_to_string(ast::decl_classifier c) {
	switch (c) {
		using enum catalyst::ast::decl_classifier;
	case abstract_:
		return "abstract";
	case public_:
		return "public";
	case private_:
		return "private";
	case protected_:
		return "protected";
	case virtual_:
		return "virtual";
	case static_:
		return "static";
	case override_:
		return "override";
	default:
		return "<unknown classifier>";
	}
}


ast::decl_classifier string_to_classifier(const std::string &str) {
	using enum catalyst::ast::decl_classifier;
	if (str == "abstract") {
		return abstract_;
	} else if (str == "public") {
		return public_;
	} else if (str == "private") {
		return private_;
	} else if (str == "protected") {
		return protected_;
	} else if (str == "virtual") {
		return virtual_;
	} else if (str == "static") {
		return static_;
	} else if (str == "override") {
		return override_;
	} else {
		parser::report_message(parser::report_type::warning, "Classifier `" + str + "` is defined on a member, but is not supported in this version.", std::cerr);
		return ast::decl_classifier::public_; // fallback
	}
}

template <typename T>
bool vector_contains(const std::vector<T> &v, const T &value) {
	return std::any_of(v.begin(), v.end(), [&](const T &i) { return i == value; });
}

template <typename T, class UnaryPredicate>
bool vector_contains(const std::vector<T> &v, UnaryPredicate p) {
	return std::any_of(v.begin(), v.end(), p);
}

bool check_decl_classifiers(codegen::state &state, const ast::decl_fn &decl) {
	int num_errors = 0;
	if (vector_contains(decl.classifiers, ast::decl_classifier::virtual_) &&
	    vector_contains(decl.classifiers, ast::decl_classifier::override_)) {
		state.report_message(report_type::error,
		                     "Function cannot be classified both `virtual` and `override`", &decl);
		num_errors++;
	}

	for (auto c : decl.classifiers) {
		if (c == ast::decl_classifier::virtual_ || c == ast::decl_classifier::override_) {
			auto potential_class_name = state.current_scope().get_fully_qualified_scope_name();
			if (!state.symbol_table.contains(potential_class_name)) {
				state.report_message(report_type::error,
				                     std::string("`") + classifier_to_string(c) +
				                         "` keyword on non-class function",
				                     &decl);
				num_errors++;
				continue;
			}
			auto symbol = state.symbol_table[potential_class_name];
			if (!isa<type_class>(symbol.type)) {
				state.report_message(report_type::error,
				                     std::string("`") + classifier_to_string(c) +
				                         "` keyword on non-class function",
				                     &decl);
				num_errors++;
				continue;
			}
			auto class_type = std::static_pointer_cast<type_class>(symbol.type);
			if (c == ast::decl_classifier::virtual_) {
				for (auto const& super : class_type->super) {
					auto vmems = super->get_virtual_members();
					if (vector_contains(vmems, [&](const member_locator &ml) {
							return ml.member->name == decl.ident.name;
						})) {
						state.report_message(
							report_type::error,
							"`virtual` declaration shadows virtual declaration in parent class",
							&decl);
						state.report_message(
							report_type::help,
							"use the 'override' keyword to override this virtual declaration");
						num_errors++;
						continue;
					}
				}
			} else if (c == ast::decl_classifier::override_) {
				if (class_type->super.empty()) {
					state.report_message(report_type::error,
					                     "Cannot `override` function in class without parent",
					                     &decl);
					num_errors++;
					continue;
				}
				
				bool found = false;
				for (auto const& super : class_type->super) {
					auto vmems = super->get_virtual_members();
					if (vector_contains(vmems, [&](const member_locator &ml) {
							return ml.member->name == decl.ident.name;
						})) {
							found = true;
							break;
					}
				}
				if (!found) {
					state.report_message(
                        report_type::error,
                        "declaration does not override a virtual function in any parent class",
                        &decl);
                    num_errors++;
                    continue;
				}
			}
		} else {
			state.report_message(report_type::error,
			                     std::string("unsupported classifier `") + classifier_to_string(c) +
			                         "` on function",
			                     &decl);
			num_errors++;
		}
	}

    // check for function without override that shadows a virtual function
    if (!vector_contains(decl.classifiers, ast::decl_classifier::override_)) {
        auto potential_class_name = state.current_scope().get_fully_qualified_scope_name();
        if (state.symbol_table.contains(potential_class_name)) {
            auto symbol = state.symbol_table[potential_class_name];
            if (isa<type_class>(symbol.type)) {
                auto class_type = std::static_pointer_cast<type_class>(symbol.type);
                if (!class_type->super.empty()) {
                    // TODO: probably also check for shadowing of non-virtual functions as well
					for (auto const &s : class_type->super) {
						auto vmems = s->get_virtual_members();
						if (vector_contains(vmems, [&](const member_locator &ml) {
								return ml.member->name == decl.ident.name;
							})) {
							state.report_message(
								report_type::error,
								"declaration shadows a virtual function",
								&decl);
							num_errors++;
						}
					}
                }
            }
        }
    }

	return num_errors == 0;
}

bool check_decl_classifiers(codegen::state &state, const ast::decl_ns &decl) {
	// no classifiers are supported for ns yet
	for (auto c : decl.classifiers) {
		state.report_message(
			report_type::error,
			std::string("unsupported classifier `") + classifier_to_string(c) + "` on namespace", &decl);
	}
	return decl.classifiers.empty();
}

bool check_decl_classifiers(codegen::state &state, const ast::decl_class &decl) {
	// no classifiers are supported for class yet
	for (auto c : decl.classifiers) {
		state.report_message(report_type::error,
		                     std::string("unsupported classifier `") + classifier_to_string(c) +
		                         "` on class",
		                     &decl);
	}
	return decl.classifiers.empty();
}

bool check_decl_classifiers(codegen::state &state, const ast::decl_struct &decl) {
	// no classifiers are supported for struct yet
	for (auto c : decl.classifiers) {
		state.report_message(report_type::error,
		                     std::string("unsupported classifier `") + classifier_to_string(c) +
		                         "` on struct",
		                     &decl);
	}
	return decl.classifiers.empty();
}

bool check_decl_classifiers(codegen::state &state, const ast::decl_var &decl) {
	// no classifiers are supported for var yet
	for (auto c : decl.classifiers) {
		state.report_message(
			report_type::error,
			std::string("unsupported classifier `") + classifier_to_string(c) + "` on variable", &decl);
	}
	return decl.classifiers.empty();
}

} // namespace catalyst::compiler::codegen
