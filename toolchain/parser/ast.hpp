#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "parser.hpp"

namespace catalyst::ast {

struct ident {
	std::string name;
	
	struct {
		parser::char_type *begin, *end;
	} lexeme;

	template <typename Input>
	auto get_input_location(const Input &input) const {
		return lexy::get_input_location(input, lexeme.begin);
	}

	template <typename Input>
	auto get_input_line_annotation(const Input &input) const {
		return lexy::get_input_line_annotation(input, get_input_location(input), lexeme.end);
	}
};

struct decl {
	//std::string a;
	ident ident;
	// const LEXY_CHAR8_T *k;
};

struct translation_unit {
	std::vector<decl> declarations;
};

} // namespace ast
