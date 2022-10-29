#include "../common/contrib/CLI11.hpp"
#include "../common/contrib/rang.hpp"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <lexy/action/parse.hpp>
#include <lexy/action/parse_as_tree.hpp>
#include <lexy/input/file.hpp>
#include <lexy/visualize.hpp>

#include <lexy_ext/report_error.hpp>

#include "../common/platform.hpp"

#include "ast.hpp"
#include "grammar.hpp"

#if CATALYST_PLATFORM_POSIX
#include <csignal>
void signal_handler(int s) {
	std::cout << std::endl << rang::style::reset << rang::fg::red << rang::fg::bold;
	std::cout << "Control-C detected, exiting..." << rang::style::reset << std::endl;
	std::exit(1); // will call the correct exit func, no unwinding of the stack though
}
#endif

namespace catalyst::parser {

struct options {
	std::string input;
	bool compile_only = false;
	bool dump_ast = false;
	bool dump_bytecode = false;

	enum class OutputFormat { ASCII = 0, Colors, Fancy } output_format = OutputFormat::ASCII;
};

int main(const options &opts) {
	auto file = lexy::read_file<lexy::deduce_encoding<parser::char_type>>(opts.input.c_str());
	if (!file) {
		std::fprintf(stderr, "file '%s' not found", opts.input.c_str());
		return 2;
	}

	if (opts.dump_ast) {
		lexy::parse_tree_for<decltype(file.buffer())> tree;
		auto result = lexy::parse_as_tree<grammar::translation_unit>(
	   		tree, file.buffer(), lexy_ext::report_error.path(opts.input.c_str()));

		switch (opts.output_format) {
		default:
		case options::OutputFormat::ASCII:
			lexy::visualize(stdout, tree, {});
			break;
		case options::OutputFormat::Colors:
			lexy::visualize(stdout, tree, {lexy::visualize_use_color});
			break;
		case options::OutputFormat::Fancy:
			lexy::visualize(stdout, tree, {lexy::visualize_fancy});
			break;
		};
	}

	// auto root = std::get<ast::json_object>(((ast::json_value *)tree.root().address())->v);

	auto json = lexy::parse<grammar::translation_unit>(
	    file.buffer(), lexy_ext::report_error.path(opts.input.c_str()));
	// auto lexeme = (lexy::buffer_lexeme<>*)json.value().ident.lexeme;
	if (json.has_value()) {
		auto il = json.value().declarations[0].ident.get_input_location(file.buffer());
		auto ila = json.value().declarations[0].ident.get_input_line_annotation(file.buffer());
		std::string annotated = std::string(ila.annotated.begin(), ila.annotated.end());
		std::string after = std::string(ila.after.begin(), ila.after.end());
		std::string before = std::string(ila.before.begin(), ila.before.end());
		lexy::visualize(stdout, ila.before, {});
		lexy::visualize(stdout, ila.annotated, {});
		lexy::visualize(stdout, ila.after, {});
		std::cout << std::endl;
		auto indent_count = lexy::visualization_display_width(ila.before, {});
		for (auto i = 0; i != indent_count; ++i)
			std::cout << ' ';
		for (auto i = 0; i != ila.annotated.size(); ++i)
			std::cout << '^';
		// std::cout << json.value().ident.name << ": " << (int)json.value().ident.lexeme.begin <<
		// std::endl;
	}

	return 0;
}

} // namespace catalyst::parser

using namespace catalyst;
using namespace catalyst::parser;

int main(int argc, char **argv) {
	CLI::App app{"Catalyst Parser"};
	options options;

	app.set_version_flag("--version",
	                     std::format("{} {} (Catalyst {})", app.get_description(),
	                                 parser::version.string(), catalyst::version.string()));

	app.add_option("input", options.input, "The Catalyst file to be parsed.")
	    ->required()
	    ->check(CLI::ExistingFile);

	app.add_flag("--dump-ast", options.dump_ast, "Dump AST to stdout.");

	std::map<std::string, options::OutputFormat> output_type_map{
	    {"ascii", options::OutputFormat::ASCII},
	    {"yes", options::OutputFormat::Colors},
	    {"fancy", options::OutputFormat::Fancy}};
	app.add_option("--output-format", options.output_format, "Set the output format")
	    ->transform(CLI::CheckedTransformer(output_type_map, CLI::ignore_case));

	std::atexit([]() { std::cout << rang::style::reset; });
#if CATALYST_PLATFORM_POSIX
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, nullptr);
#endif
	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError &e) {
		std::cout << (e.get_exit_code() == 0 ? rang::fg::blue : rang::fg::red);
		return app.exit(e);
	}

	return main(options);
}
