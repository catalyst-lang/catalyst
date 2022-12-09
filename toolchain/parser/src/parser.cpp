#include <cinttypes>
#include <codecvt>
#include <cstdint>
#include <cstdio>
#include <locale>
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

#include <rang.hpp>

#include <iostream>
#include <lexy_ext/report_error.hpp>

#include "catalyst/platform.hpp"

#include "grammar.hpp"
#include "parser.hpp"
#include "state.hpp"

namespace catalyst::parser {

//auto _report_error = lexy_ext::report_message.opts({lexy::visualization_flags::visualize_use_color});
auto _report_error = lexy_ext::report_error.opts({});

// template <typename Input>
std::optional<ast::translation_unit> parse(parser_state_ptr state) {
	auto re = lexy_ext::report_error.opts({});
	auto ast = lexy::parse<grammar::translation_unit>(state->input, _report_error);
	if (!ast.is_error()) {
		ast::translation_unit tu = ast.value();
		tu.parser_state = state;
		return tu;
	} else {
		return std::nullopt;
	}
}

std::optional<ast::translation_unit> parse_string(const std::string &string) {
	auto state = std::make_shared<parser::parser_state>();
	auto input = lexy::string_input<lexy::deduce_encoding<parser::char_type>>(string);

	constexpr auto make_le = lexy::make_buffer_from_raw<lexy::deduce_encoding<parser::char_type>,
	                                                    lexy::encoding_endianness::little>;

	state->input = make_le(string.data(), string.size());

	return parse(state);
}

std::optional<ast::translation_unit> parse_filename(const std::string &filename) {
	auto file = lexy::read_file<lexy::deduce_encoding<parser::char_type>>(filename.c_str());
	if (!file) {
		std::fprintf(stderr, "file '%s' not found", filename.c_str());
		return std::nullopt;
	}
	auto state = std::make_shared<parser::parser_state>();
	state->input = file.buffer();
	state->filename = filename;

	return parse(state);
}

/// Error handling

struct production {
	static constexpr auto name = "production";
	static constexpr auto rule = 0; // Need a rule member to make it a production.
};

void report_message(report_type t, const std::string &message) {
	switch (t) {
	case report_type::error:
		std::cout << rang::fg::red << "Error" << rang::fg::reset << ": ";
		break;
	case report_type::warning:
		std::cout << rang::fg::yellow << "Warning" << rang::fg::reset << ": ";
		break;
	case report_type::info:
		std::cout << rang::fg::blue << "Info" << rang::fg::reset << ": ";
		break;
	case report_type::debug:
		std::cout << "Debug" << ": ";
		break;
	case report_type::help:
		std::cout << rang::fg::green << "Help" << rang::fg::reset << ": ";
		break;
	}
	std::cout << message << std::endl;
}

void report_message(report_type type, parser_state_ptr parser_state,
                    const std::string &message_title, const parser::ast_node &positional,
                    const std::string &message_positional_title) {
	auto write = [parser_state](const auto &context, const auto &message) {
		std::wstring str;
		// lexy_ext::_detail::write_error(std::back_insert_iterator(str), context, message,
		// {lexy::visualization_flags::visualize_use_color}, nullptr);
		const char *filename =
			parser_state->filename == "" ? nullptr : parser_state->filename.c_str();
		lexy_ext::_detail::write_error(std::back_insert_iterator(str), context, message, {},
		                               filename);
		return str.substr(str.find_first_of('\n') + 1);
	};
	// auto context =
	//	lexy::error_context(production{}, parser_state->input, parser_state->input.data());
	auto context = lexy::error_context(production{}, parser_state->input, positional.lexeme.begin);
	auto error = lexy::error<lexy::_pr8, void>(positional.lexeme.begin, positional.lexeme.end,
	                                           message_positional_title.c_str());

	report_message(type, message_title);

	auto out = write(context, error);
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	std::cout << converter.to_bytes(out) << std::endl;
}

} // namespace catalyst::parser
