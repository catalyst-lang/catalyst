#include <contrib/CLI11.hpp>
#include <contrib/rang.hpp>

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

std::optional<ast::translation_unit> parse(const std::string &string) {
	auto input = lexy::string_input<lexy::deduce_encoding<parser::char_type>>(string);
	auto ast = lexy::parse<grammar::translation_unit>(input, lexy_ext::report_error);
	if (!ast.is_error()) {
		return ast.value();
	} else {
		return std::nullopt;
	}
}

} // namespace catalyst::parser
