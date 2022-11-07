#include <CLI11.hpp>
#include <rang.hpp>

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
#include <lexy/visualize.hpp>

#include <lexy_ext/report_error.hpp>

#include "catalyst/platform.hpp"

#include "catalyst/ast/ast.hpp"
#include "grammar.hpp"
#include "parser.hpp"

#if CATALYST_PLATFORM_POSIX
#include <csignal>
void signal_handler(int s) {
	std::cout << std::endl << rang::style::reset << rang::fg::red << rang::style::bold;
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

	enum class OutputFormat { ASCII = 0, Color, Fancy } format = OutputFormat::ASCII;
};

int main(const options &opts) {
	if (opts.dump_ast) {
		auto file = lexy::read_file<lexy::deduce_encoding<parser::char_type>>(opts.input.c_str());
		if (!file) {
			std::fprintf(stderr, "file '%s' not found", opts.input.c_str());
			return 2;
		}
		lexy::parse_tree_for<decltype(file.buffer())> tree;
		auto result = lexy::parse_as_tree<grammar::translation_unit>(
		    tree, file.buffer(), lexy_ext::report_error.path(opts.input.c_str()));

		switch (opts.format) {
			using enum catalyst::parser::options::OutputFormat;
			default:
			case ASCII:
				lexy::visualize(stdout, tree, {});
				break;
			case Color:
				lexy::visualize(stdout, tree, {lexy::visualize_use_color});
				break;
			case Fancy:
				lexy::visualize(stdout, tree, {lexy::visualize_fancy});
				break;
		}
	}

	auto ast = parse_filename(opts.input.c_str());
	if (ast.has_value()) {
		auto fn = (ast::decl_fn*) ast.value().declarations[0].get();
        auto body = std::get<ast::fn_body_block>(fn->body);

		/*auto il = ast.value().declarations[0]->ident.get_input_location(file.buffer());
		auto ila = ast.value().declarations[0].ident.get_input_line_annotation(file.buffer());
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
		std::cout << std::endl;
		*/
		std::cout << std::endl;
	} else {
		std::cout << rang::fg::red << "Errors occurred." << rang::fg::reset << std::endl;
		return 1;
	}

	return 0;
}

} // namespace catalyst::parser

using namespace catalyst;
using namespace catalyst::parser;

int main(int argc, char **argv) {
	CLI::App app{"Catalyst Parser"};
	options options;

	std::stringstream version_string;
	version_string << app.get_description() << " " << parser::version.string() << "(Catalyst "
	               << catalyst::version.string() << ")";
	app.set_version_flag("--version", version_string.str());

	app.add_option("input", options.input, "The Catalyst file to be parsed.")
	    ->required()
	    ->check(CLI::ExistingFile);

	app.add_flag("--dump-ast", options.dump_ast, "Dump AST to stdout.");

	std::map<std::string, options::OutputFormat> output_type_map{
	    {"ascii", options::OutputFormat::ASCII},
	    {"color", options::OutputFormat::Color},
	    {"fancy", options::OutputFormat::Fancy}};
	app.add_option("--format", options.format, "Set the output format")
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
