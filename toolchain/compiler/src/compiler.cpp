//
// Created by basdu on 4-11-2022.
//

#include "compiler.hpp"
#include "../../parser/src/parser.hpp"

namespace catalyst::compiler {

bool compile(catalyst::ast::translation_unit &tu) {
	return false;
}

bool compile(const std::string &filename) {
	auto ast = parser::parse_filename(filename);
	if (ast.has_value()) {
		return compile(ast.value());
	} else {
		return false;
	}
}

} // namespace catalyst