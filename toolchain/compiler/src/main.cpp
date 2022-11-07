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

#include "catalyst/platform.hpp"
#include "catalyst/version.hpp"
#include "compiler.hpp"

#if CATALYST_PLATFORM_POSIX
#include <csignal>
void signal_handler(int s) {
	std::cout << std::endl << rang::style::reset << rang::fg::red << rang::style::bold;
	std::cout << "Control-C detected, exiting..." << rang::style::reset << std::endl;
	std::exit(1); // will call the correct exit func, no unwinding of the stack though
}
#endif

namespace catalyst::compiler {

struct options {
	std::string input;
	bool compile_only = false;
	bool dump_ast = false;
	bool dump_bytecode = false;

	enum class OutputFormat { ASCII = 0, Color, Fancy } format = OutputFormat::ASCII;
};

int main(const options &opts) {
	return compile(opts.input.c_str()) ? 0 : 1;
}

} // namespace catalyst::parser

using namespace catalyst;
using namespace catalyst::compiler;

int main(int argc, char **argv) {
	CLI::App app{"Catalyst Compiler"};
	options options;

	std::stringstream version_string;
	version_string << app.get_description() << " " << compiler::version.string() << "(Catalyst "
				   << catalyst::version.string() << ")";
	app.set_version_flag("--version", version_string.str());

	app.add_option("input", options.input, "The Catalyst file to be parsed.")
		->required()
		->check(CLI::ExistingFile);

	//app.add_flag("--dump-ast", options.dump_ast, "Dump AST to stdout.");

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
