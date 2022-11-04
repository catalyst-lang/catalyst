#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <lexy/action/parse.hpp>
#include <lexy/action/parse_as_tree.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy/visualize.hpp>

#include <lexy_ext/report_error.hpp>

#include "catalyst/platform.hpp"

#include "grammar.hpp"

namespace catalyst::parser {

template <typename Input>
std::optional<ast::translation_unit> parse(Input input) {
	auto ast = lexy::parse<grammar::translation_unit>(input, lexy_ext::report_error);
	if (!ast.is_error()) {
		return ast.value();
	} else {
		return std::nullopt;
	}
}

std::optional<ast::translation_unit> parse_string(const std::string &string) {
	auto input = lexy::string_input<lexy::deduce_encoding<parser::char_type>>(string);
	return parse(input);
}

std::optional<ast::translation_unit> parse_filename(const std::string &filename) {
	auto file = lexy::read_file<lexy::deduce_encoding<parser::char_type>>(filename.c_str());
	if (!file) {
		std::fprintf(stderr, "file '%s' not found", filename.c_str());
		return std::nullopt;
	}

	return parse(file.buffer());
}

} // namespace catalyst::parser
